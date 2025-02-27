#include <scwx/qt/view/level2_product_view.hpp>
#include <scwx/qt/util/geographic_lib.hpp>
#include <scwx/common/constants.hpp>
#include <scwx/util/logger.hpp>
#include <scwx/util/threads.hpp>
#include <scwx/util/time.hpp>

#include <boost/range/irange.hpp>
#include <boost/timer/timer.hpp>

namespace scwx
{
namespace qt
{
namespace view
{

static const std::string logPrefix_ = "scwx::qt::view::level2_product_view";
static const auto        logger_    = scwx::util::Logger::Create(logPrefix_);

static constexpr std::uint32_t kMaxRadialGates_ =
   common::MAX_0_5_DEGREE_RADIALS * common::MAX_DATA_MOMENT_GATES;
static constexpr std::uint32_t kMaxCoordinates_ = kMaxRadialGates_ * 2u;

static constexpr uint16_t RANGE_FOLDED      = 1u;
static constexpr uint32_t VERTICES_PER_BIN  = 6u;
static constexpr uint32_t VALUES_PER_VERTEX = 2u;

static const std::unordered_map<common::Level2Product,
                                wsr88d::rda::DataBlockType>
   blockTypes_ {
      {common::Level2Product::Reflectivity,
       wsr88d::rda::DataBlockType::MomentRef},
      {common::Level2Product::Velocity, wsr88d::rda::DataBlockType::MomentVel},
      {common::Level2Product::SpectrumWidth,
       wsr88d::rda::DataBlockType::MomentSw},
      {common::Level2Product::DifferentialReflectivity,
       wsr88d::rda::DataBlockType::MomentZdr},
      {common::Level2Product::DifferentialPhase,
       wsr88d::rda::DataBlockType::MomentPhi},
      {common::Level2Product::CorrelationCoefficient,
       wsr88d::rda::DataBlockType::MomentRho},
      {common::Level2Product::ClutterFilterPowerRemoved,
       wsr88d::rda::DataBlockType::MomentCfp}};

class Level2ProductViewImpl
{
public:
   explicit Level2ProductViewImpl(Level2ProductView*    self,
                                  common::Level2Product product) :
       self_ {self},
       product_ {product},
       selectedElevation_ {0.0f},
       elevationScan_ {nullptr},
       momentDataBlock0_ {nullptr},
       latitude_ {},
       longitude_ {},
       elevationCut_ {},
       elevationCuts_ {},
       range_ {},
       vcp_ {},
       sweepTime_ {},
       colorTable_ {},
       colorTableLut_ {},
       colorTableMin_ {2},
       colorTableMax_ {254},
       savedColorTable_ {nullptr},
       savedScale_ {0.0f},
       savedOffset_ {0.0f}
   {
      coordinates_.resize(kMaxCoordinates_);

      SetProduct(product);
   }
   ~Level2ProductViewImpl() = default;

   void
   ComputeCoordinates(std::shared_ptr<wsr88d::rda::ElevationScan> radarData);

   void SetProduct(const std::string& productName);
   void SetProduct(common::Level2Product product);

   Level2ProductView* self_;

   common::Level2Product      product_;
   wsr88d::rda::DataBlockType dataBlockType_;

   float selectedElevation_;

   std::shared_ptr<wsr88d::rda::ElevationScan>   elevationScan_;
   std::shared_ptr<wsr88d::rda::MomentDataBlock> momentDataBlock0_;

   std::vector<float>    coordinates_ {};
   std::vector<float>    vertices_ {};
   std::vector<uint8_t>  dataMoments8_ {};
   std::vector<uint16_t> dataMoments16_ {};
   std::vector<uint8_t>  cfpMoments_ {};

   float              latitude_;
   float              longitude_;
   float              elevationCut_;
   std::vector<float> elevationCuts_;
   float              range_;
   uint16_t           vcp_;

   std::chrono::system_clock::time_point sweepTime_;

   std::shared_ptr<common::ColorTable>    colorTable_;
   std::vector<boost::gil::rgba8_pixel_t> colorTableLut_;
   uint16_t                               colorTableMin_;
   uint16_t                               colorTableMax_;

   std::shared_ptr<common::ColorTable> savedColorTable_;
   float                               savedScale_;
   float                               savedOffset_;
};

Level2ProductView::Level2ProductView(
   common::Level2Product                         product,
   std::shared_ptr<manager::RadarProductManager> radarProductManager) :
    RadarProductView(radarProductManager),
    p(std::make_unique<Level2ProductViewImpl>(this, product))
{
   ConnectRadarProductManager();
}

Level2ProductView::~Level2ProductView()
{
   std::unique_lock sweepLock {sweep_mutex()};
}

void Level2ProductView::ConnectRadarProductManager()
{
   connect(radar_product_manager().get(),
           &manager::RadarProductManager::DataReloaded,
           this,
           [this](std::shared_ptr<types::RadarProductRecord> record)
           {
              if (record->radar_product_group() ==
                     common::RadarProductGroup::Level2 &&
                  std::chrono::floor<std::chrono::seconds>(record->time()) ==
                     selected_time())
              {
                 // If the data associated with the currently selected time is
                 // reloaded, update the view
                 Update();
              }
           });
}

void Level2ProductView::DisconnectRadarProductManager()
{
   disconnect(radar_product_manager().get(),
              &manager::RadarProductManager::DataReloaded,
              this,
              nullptr);
}

const std::vector<boost::gil::rgba8_pixel_t>&
Level2ProductView::color_table() const
{
   if (p->colorTableLut_.size() == 0)
   {
      return RadarProductView::color_table();
   }
   else
   {
      return p->colorTableLut_;
   }
}

uint16_t Level2ProductView::color_table_min() const
{
   if (p->colorTableLut_.size() == 0)
   {
      return RadarProductView::color_table_min();
   }
   else
   {
      return p->colorTableMin_;
   }
}

uint16_t Level2ProductView::color_table_max() const
{
   if (p->colorTableLut_.size() == 0)
   {
      return RadarProductView::color_table_max();
   }
   else
   {
      return p->colorTableMax_;
   }
}

float Level2ProductView::elevation() const
{
   return p->elevationCut_;
}

float Level2ProductView::range() const
{
   return p->range_;
}

std::chrono::system_clock::time_point Level2ProductView::sweep_time() const
{
   return p->sweepTime_;
}

uint16_t Level2ProductView::vcp() const
{
   return p->vcp_;
}

const std::vector<float>& Level2ProductView::vertices() const
{
   return p->vertices_;
}

common::RadarProductGroup Level2ProductView::GetRadarProductGroup() const
{
   return common::RadarProductGroup::Level2;
}

std::string Level2ProductView::GetRadarProductName() const
{
   return common::GetLevel2Name(p->product_);
}

std::vector<float> Level2ProductView::GetElevationCuts() const
{
   return p->elevationCuts_;
}

std::tuple<const void*, size_t, size_t> Level2ProductView::GetMomentData() const
{
   const void* data;
   size_t      dataSize;
   size_t      componentSize;

   if (p->dataMoments8_.size() > 0)
   {
      data          = p->dataMoments8_.data();
      dataSize      = p->dataMoments8_.size() * sizeof(uint8_t);
      componentSize = 1;
   }
   else
   {
      data          = p->dataMoments16_.data();
      dataSize      = p->dataMoments16_.size() * sizeof(uint16_t);
      componentSize = 2;
   }

   return std::tie(data, dataSize, componentSize);
}

std::tuple<const void*, size_t, size_t>
Level2ProductView::GetCfpMomentData() const
{
   const void* data          = nullptr;
   size_t      dataSize      = 0;
   size_t      componentSize = 1;

   if (p->cfpMoments_.size() > 0)
   {
      data     = p->cfpMoments_.data();
      dataSize = p->cfpMoments_.size() * sizeof(uint8_t);
   }

   return std::tie(data, dataSize, componentSize);
}

void Level2ProductView::LoadColorTable(
   std::shared_ptr<common::ColorTable> colorTable)
{
   p->colorTable_ = colorTable;
   UpdateColorTable();
}

void Level2ProductView::SelectElevation(float elevation)
{
   p->selectedElevation_ = elevation;
}

void Level2ProductView::SelectProduct(const std::string& productName)
{
   p->SetProduct(productName);
}

void Level2ProductViewImpl::SetProduct(const std::string& productName)
{
   SetProduct(common::GetLevel2Product(productName));
}

void Level2ProductViewImpl::SetProduct(common::Level2Product product)
{
   product_ = product;

   auto it = blockTypes_.find(product);

   if (it != blockTypes_.end())
   {
      dataBlockType_ = it->second;
   }
   else
   {
      logger_->warn("Unknown product: \"{}\"", common::GetLevel2Name(product));
      dataBlockType_ = wsr88d::rda::DataBlockType::Unknown;
   }
}

void Level2ProductView::UpdateColorTable()
{
   if (p->momentDataBlock0_ == nullptr || //
       p->colorTable_ == nullptr ||       //
       !p->colorTable_->IsValid())
   {
      // Nothing to update
      return;
   }

   float offset = p->momentDataBlock0_->offset();
   float scale  = p->momentDataBlock0_->scale();

   if (p->savedColorTable_ == p->colorTable_ && //
       p->savedOffset_ == offset &&             //
       p->savedScale_ == scale)
   {
      // The color table LUT does not need updated
      return;
   }

   uint16_t rangeMin;
   uint16_t rangeMax;

   switch (p->product_)
   {
   case common::Level2Product::Reflectivity:
   case common::Level2Product::Velocity:
   case common::Level2Product::SpectrumWidth:
   case common::Level2Product::CorrelationCoefficient:
   default:
      rangeMin = 1;
      rangeMax = 255;
      break;

   case common::Level2Product::DifferentialReflectivity:
      rangeMin = 1;
      rangeMax = 1058;
      break;

   case common::Level2Product::DifferentialPhase:
      rangeMin = 1;
      rangeMax = 1023;
      break;

   case common::Level2Product::ClutterFilterPowerRemoved:
      rangeMin = 1;
      rangeMax = 81;
      break;
   }

   boost::integer_range<uint16_t> dataRange =
      boost::irange<uint16_t>(rangeMin, rangeMax + 1);

   std::vector<boost::gil::rgba8_pixel_t>& lut = p->colorTableLut_;
   lut.resize(rangeMax - rangeMin + 1);
   lut.shrink_to_fit();

   std::for_each(std::execution::par_unseq,
                 dataRange.begin(),
                 dataRange.end(),
                 [&](uint16_t i)
                 {
                    if (i == RANGE_FOLDED)
                    {
                       lut[i - *dataRange.begin()] = p->colorTable_->rf_color();
                    }
                    else
                    {
                       float f                     = (i - offset) / scale;
                       lut[i - *dataRange.begin()] = p->colorTable_->Color(f);
                    }
                 });

   p->colorTableMin_ = rangeMin;
   p->colorTableMax_ = rangeMax;

   p->savedColorTable_ = p->colorTable_;
   p->savedOffset_     = offset;
   p->savedScale_      = scale;

   Q_EMIT ColorTableUpdated();
}

void Level2ProductView::ComputeSweep()
{
   logger_->debug("ComputeSweep()");

   boost::timer::cpu_timer timer;

   if (p->dataBlockType_ == wsr88d::rda::DataBlockType::Unknown)
   {
      Q_EMIT SweepNotComputed(types::NoUpdateReason::InvalidProduct);
      return;
   }

   std::scoped_lock sweepLock(sweep_mutex());

   std::shared_ptr<manager::RadarProductManager> radarProductManager =
      radar_product_manager();

   std::shared_ptr<wsr88d::rda::ElevationScan> radarData;
   std::chrono::system_clock::time_point       requestedTime {selected_time()};
   std::chrono::system_clock::time_point       foundTime;
   std::tie(radarData, p->elevationCut_, p->elevationCuts_, foundTime) =
      radarProductManager->GetLevel2Data(
         p->dataBlockType_, p->selectedElevation_, requestedTime);

   // If a different time was found than what was requested, update it
   if (requestedTime != foundTime)
   {
      SelectTime(foundTime);
   }

   if (radarData == nullptr)
   {
      Q_EMIT SweepNotComputed(types::NoUpdateReason::NotLoaded);
      return;
   }
   if (radarData == p->elevationScan_)
   {
      Q_EMIT SweepNotComputed(types::NoUpdateReason::NoChange);
      return;
   }

   const size_t radials = radarData->size();

   p->ComputeCoordinates(radarData);

   const std::vector<float>& coordinates = p->coordinates_;

   auto& radarData0     = (*radarData)[0];
   auto  momentData0    = radarData0->moment_data_block(p->dataBlockType_);
   p->elevationScan_    = radarData;
   p->momentDataBlock0_ = momentData0;

   if (momentData0 == nullptr)
   {
      logger_->warn("No moment data for {}",
                    common::GetLevel2Name(p->product_));
      Q_EMIT SweepNotComputed(types::NoUpdateReason::InvalidData);
      return;
   }

   const uint32_t gates = momentData0->number_of_data_moment_gates();

   auto radialData0 = radarData0->radial_data_block();
   auto volumeData0 = radarData0->volume_data_block();
   p->latitude_     = volumeData0->latitude();
   p->longitude_    = volumeData0->longitude();
   p->range_ =
      momentData0->data_moment_range() +
      momentData0->data_moment_range_sample_interval() * (gates - 0.5f);
   p->sweepTime_ = scwx::util::TimePoint(radarData0->modified_julian_date(),
                                         radarData0->collection_time());
   p->vcp_       = volumeData0->volume_coverage_pattern_number();

   // Calculate vertices
   timer.start();

   // Setup vertex vector
   std::vector<float>& vertices = p->vertices_;
   size_t              vIndex   = 0;
   vertices.clear();
   vertices.resize(radials * gates * VERTICES_PER_BIN * VALUES_PER_VERTEX);

   // Setup data moment vector
   std::vector<uint8_t>&  dataMoments8  = p->dataMoments8_;
   std::vector<uint16_t>& dataMoments16 = p->dataMoments16_;
   std::vector<uint8_t>&  cfpMoments    = p->cfpMoments_;
   size_t                 mIndex        = 0;

   if (momentData0->data_word_size() == 8)
   {
      dataMoments16.resize(0);
      dataMoments16.shrink_to_fit();

      dataMoments8.resize(radials * gates * VERTICES_PER_BIN);
   }
   else
   {
      dataMoments8.resize(0);
      dataMoments8.shrink_to_fit();

      dataMoments16.resize(radials * gates * VERTICES_PER_BIN);
   }

   if (p->dataBlockType_ == wsr88d::rda::DataBlockType::MomentRef &&
       radarData0->moment_data_block(wsr88d::rda::DataBlockType::MomentCfp) !=
          nullptr)
   {
      cfpMoments.resize(radials * gates * VERTICES_PER_BIN);
   }
   else
   {
      cfpMoments.resize(0);
      cfpMoments.shrink_to_fit();
   }

   // Compute threshold at which to display an individual bin (minimum of 2)
   const uint16_t snrThreshold =
      std::max<int16_t>(2, momentData0->snr_threshold_raw());

   // Start radial is always 0, as coordinates are calculated for each sweep
   constexpr std::uint16_t startRadial = 0u;

   for (auto& radialPair : *radarData)
   {
      uint16_t radial     = radialPair.first;
      auto     radialData = radialPair.second;
      auto     momentData = radialData->moment_data_block(p->dataBlockType_);

      if (momentData0->data_word_size() != momentData->data_word_size())
      {
         logger_->warn("Radial {} has different word size", radial);
         continue;
      }

      // Compute gate interval
      const uint16_t dataMomentRange = momentData->data_moment_range_raw();
      const uint16_t dataMomentInterval =
         momentData->data_moment_range_sample_interval_raw();
      const uint16_t dataMomentIntervalH = dataMomentInterval / 2;

      // Compute gate size (number of base 250m gates per bin)
      const uint16_t gateSizeMeters =
         static_cast<uint16_t>(radarProductManager->gate_size());
      const uint16_t gateSize =
         std::max<uint16_t>(1, dataMomentInterval / gateSizeMeters);

      // Compute gate range [startGate, endGate)
      const uint16_t startGate =
         (dataMomentRange - dataMomentIntervalH) / gateSizeMeters;
      const uint16_t numberOfDataMomentGates =
         std::min<uint16_t>(momentData->number_of_data_moment_gates(),
                            static_cast<uint16_t>(gates));
      const uint16_t endGate =
         std::min<uint16_t>(startGate + numberOfDataMomentGates * gateSize,
                            common::MAX_DATA_MOMENT_GATES);

      const uint8_t*  dataMomentsArray8  = nullptr;
      const uint16_t* dataMomentsArray16 = nullptr;
      const uint8_t*  cfpMomentsArray    = nullptr;

      if (momentData->data_word_size() == 8)
      {
         dataMomentsArray8 =
            reinterpret_cast<const uint8_t*>(momentData->data_moments());
      }
      else
      {
         dataMomentsArray16 =
            reinterpret_cast<const uint16_t*>(momentData->data_moments());
      }

      if (cfpMoments.size() > 0)
      {
         cfpMomentsArray = reinterpret_cast<const uint8_t*>(
            radialData->moment_data_block(wsr88d::rda::DataBlockType::MomentCfp)
               ->data_moments());
      }

      for (uint16_t gate = startGate, i = 0; gate + gateSize <= endGate;
           gate += gateSize, ++i)
      {
         size_t vertexCount = (gate > 0) ? 6 : 3;

         // Store data moment value
         if (dataMomentsArray8 != nullptr)
         {
            uint8_t dataValue = dataMomentsArray8[i];
            if (dataValue < snrThreshold && dataValue != RANGE_FOLDED)
            {
               continue;
            }

            for (size_t m = 0; m < vertexCount; m++)
            {
               dataMoments8[mIndex++] = dataMomentsArray8[i];

               if (cfpMomentsArray != nullptr)
               {
                  cfpMoments[mIndex - 1] = cfpMomentsArray[i];
               }
            }
         }
         else
         {
            uint16_t dataValue = dataMomentsArray16[i];
            if (dataValue < snrThreshold && dataValue != RANGE_FOLDED)
            {
               continue;
            }

            for (size_t m = 0; m < vertexCount; m++)
            {
               dataMoments16[mIndex++] = dataMomentsArray16[i];
            }
         }

         // Store vertices
         if (gate > 0)
         {
            const uint16_t baseCoord = gate - 1;

            size_t offset1 = ((startRadial + radial) % radials *
                                 common::MAX_DATA_MOMENT_GATES +
                              baseCoord) *
                             2;
            size_t offset2 = offset1 + gateSize * 2;
            size_t offset3 = (((startRadial + radial + 1) % radials) *
                                 common::MAX_DATA_MOMENT_GATES +
                              baseCoord) *
                             2;
            size_t offset4 = offset3 + gateSize * 2;

            vertices[vIndex++] = coordinates[offset1];
            vertices[vIndex++] = coordinates[offset1 + 1];

            vertices[vIndex++] = coordinates[offset2];
            vertices[vIndex++] = coordinates[offset2 + 1];

            vertices[vIndex++] = coordinates[offset3];
            vertices[vIndex++] = coordinates[offset3 + 1];

            vertices[vIndex++] = coordinates[offset3];
            vertices[vIndex++] = coordinates[offset3 + 1];

            vertices[vIndex++] = coordinates[offset4];
            vertices[vIndex++] = coordinates[offset4 + 1];

            vertices[vIndex++] = coordinates[offset2];
            vertices[vIndex++] = coordinates[offset2 + 1];

            vertexCount = 6;
         }
         else
         {
            const uint16_t baseCoord = gate;

            size_t offset1 = ((startRadial + radial) % radials *
                                 common::MAX_DATA_MOMENT_GATES +
                              baseCoord) *
                             2;
            size_t offset2 = (((startRadial + radial + 1) % radials) *
                                 common::MAX_DATA_MOMENT_GATES +
                              baseCoord) *
                             2;

            vertices[vIndex++] = p->latitude_;
            vertices[vIndex++] = p->longitude_;

            vertices[vIndex++] = coordinates[offset1];
            vertices[vIndex++] = coordinates[offset1 + 1];

            vertices[vIndex++] = coordinates[offset2];
            vertices[vIndex++] = coordinates[offset2 + 1];

            vertexCount = 3;
         }
      }
   }
   vertices.resize(vIndex);
   vertices.shrink_to_fit();

   if (momentData0->data_word_size() == 8)
   {
      dataMoments8.resize(mIndex);
      dataMoments8.shrink_to_fit();
   }
   else
   {
      dataMoments16.resize(mIndex);
      dataMoments16.shrink_to_fit();
   }

   if (cfpMoments.size() > 0)
   {
      cfpMoments.resize(mIndex);
      cfpMoments.shrink_to_fit();
   }

   timer.stop();
   logger_->debug("Vertices calculated in {}", timer.format(6, "%ws"));

   UpdateColorTable();

   Q_EMIT SweepComputed();
}

void Level2ProductViewImpl::ComputeCoordinates(
   std::shared_ptr<wsr88d::rda::ElevationScan> radarData)
{
   logger_->debug("ComputeCoordinates()");

   boost::timer::cpu_timer timer;

   const GeographicLib::Geodesic& geodesic(
      util::GeographicLib::DefaultGeodesic());

   auto         radarProductManager = self_->radar_product_manager();
   auto         radarSite           = radarProductManager->radar_site();
   const float  gateSize            = radarProductManager->gate_size();
   const double radarLatitude       = radarSite->latitude();
   const double radarLongitude      = radarSite->longitude();

   // Calculate azimuth coordinates
   timer.start();

   auto& radarData0  = (*radarData)[0];
   auto  momentData0 = radarData0->moment_data_block(dataBlockType_);

   const std::uint16_t numRadials =
      static_cast<std::uint16_t>(radarData->size());
   const std::uint16_t numRangeBins =
      std::max(momentData0->number_of_data_moment_gates() + 1u,
               common::MAX_DATA_MOMENT_GATES);

   auto radials = boost::irange<std::uint32_t>(0u, numRadials);
   auto gates   = boost::irange<std::uint32_t>(0u, numRangeBins);

   std::for_each(
      std::execution::par_unseq,
      radials.begin(),
      radials.end(),
      [&](std::uint32_t radial)
      {
         // Angles are ordered clockwise, delta should be positive. Only correct
         // less than -90 degrees, this should cover any "overlap" scenarios.
         float deltaAngle =
            (radial == 0) ? (*radarData)[0]->azimuth_angle() -
                               (*radarData)[numRadials - 1]->azimuth_angle() :
                            (*radarData)[radial]->azimuth_angle() -
                               (*radarData)[radial - 1]->azimuth_angle();
         while (deltaAngle < -90.0f)
         {
            deltaAngle += 360.0f;
         }

         const float angle =
            (*radarData)[radial]->azimuth_angle() - (deltaAngle * 0.5f);

         std::for_each(std::execution::par_unseq,
                       gates.begin(),
                       gates.end(),
                       [&](std::uint32_t gate)
                       {
                          const std::uint32_t radialGate =
                             radial * common::MAX_DATA_MOMENT_GATES + gate;
                          const float       range  = (gate + 1) * gateSize;
                          const std::size_t offset = radialGate * 2;

                          double latitude;
                          double longitude;

                          geodesic.Direct(radarLatitude,
                                          radarLongitude,
                                          angle,
                                          range,
                                          latitude,
                                          longitude);

                          coordinates_[offset]     = latitude;
                          coordinates_[offset + 1] = longitude;
                       });
      });
   timer.stop();
   logger_->debug("Coordinates calculated in {}", timer.format(6, "%ws"));
}

std::shared_ptr<Level2ProductView> Level2ProductView::Create(
   common::Level2Product                         product,
   std::shared_ptr<manager::RadarProductManager> radarProductManager)
{
   return std::make_shared<Level2ProductView>(product, radarProductManager);
}

} // namespace view
} // namespace qt
} // namespace scwx
