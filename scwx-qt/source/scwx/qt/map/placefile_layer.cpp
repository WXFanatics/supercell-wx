#include <scwx/qt/map/placefile_layer.hpp>
#include <scwx/qt/gl/draw/placefile_icons.hpp>
#include <scwx/qt/manager/placefile_manager.hpp>
#include <scwx/qt/manager/resource_manager.hpp>
#include <scwx/qt/manager/settings_manager.hpp>
#include <scwx/qt/util/geographic_lib.hpp>
#include <scwx/qt/util/maplibre.hpp>
#include <scwx/util/logger.hpp>

#include <fmt/format.h>
#include <imgui.h>
#include <mbgl/util/constants.hpp>

namespace scwx
{
namespace qt
{
namespace map
{

static const std::string logPrefix_ = "scwx::qt::map::placefile_layer";
static const auto        logger_    = scwx::util::Logger::Create(logPrefix_);

class PlacefileLayer::Impl
{
public:
   explicit Impl(PlacefileLayer*             self,
                 std::shared_ptr<MapContext> context,
                 const std::string&          placefileName) :
       self_ {self},
       placefileName_ {placefileName},
       placefileIcons_ {std::make_shared<gl::draw::PlacefileIcons>(context)}
   {
      ConnectSignals();
   }
   ~Impl() = default;

   void ConnectSignals();

   void
   RenderIconDrawItem(const QMapLibreGL::CustomLayerRenderParameters& params,
                      const std::shared_ptr<gr::Placefile::IconDrawItem>& di);
   void
   RenderTextDrawItem(const QMapLibreGL::CustomLayerRenderParameters& params,
                      const std::shared_ptr<gr::Placefile::TextDrawItem>& di);

   void RenderText(const QMapLibreGL::CustomLayerRenderParameters& params,
                   const std::string&                              text,
                   const std::string&                              hoverText,
                   boost::gil::rgba8_pixel_t                       color,
                   float                                           x,
                   float                                           y);

   PlacefileLayer* self_;

   std::string placefileName_;
   bool        dirty_ {true};

   std::uint32_t textId_ {};
   glm::vec2     mapScreenCoordLocation_ {};
   float         mapScale_ {1.0f};
   float         halfWidth_ {};
   float         halfHeight_ {};
   bool          thresholded_ {true};
   ImFont*       monospaceFont_ {};

   std::shared_ptr<gl::draw::PlacefileIcons> placefileIcons_;
};

PlacefileLayer::PlacefileLayer(std::shared_ptr<MapContext> context,
                               const std::string&          placefileName) :
    DrawLayer(context),
    p(std::make_unique<PlacefileLayer::Impl>(this, context, placefileName))
{
   AddDrawItem(p->placefileIcons_);
}

PlacefileLayer::~PlacefileLayer() = default;

void PlacefileLayer::Impl::ConnectSignals()
{
   auto placefileManager = manager::PlacefileManager::Instance();

   QObject::connect(placefileManager.get(),
                    &manager::PlacefileManager::PlacefileUpdated,
                    self_,
                    [this](const std::string& name)
                    {
                       if (name == placefileName_)
                       {
                          dirty_ = true;
                       }
                    });
}

std::string PlacefileLayer::placefile_name() const
{
   return p->placefileName_;
}

void PlacefileLayer::set_placefile_name(const std::string& placefileName)
{
   p->placefileName_ = placefileName;
   p->dirty_         = true;
}

void PlacefileLayer::Initialize()
{
   logger_->debug("Initialize()");

   DrawLayer::Initialize();
}

void PlacefileLayer::Impl::RenderIconDrawItem(
   const QMapLibreGL::CustomLayerRenderParameters&     params,
   const std::shared_ptr<gr::Placefile::IconDrawItem>& di)
{
   if (!dirty_)
   {
      return;
   }

   auto distance =
      (thresholded_) ?
         util::GeographicLib::GetDistance(
            params.latitude, params.longitude, di->latitude_, di->longitude_) :
         0;

   if (distance < di->threshold_)
   {
      placefileIcons_->AddIcon(di);
   }
}

void PlacefileLayer::Impl::RenderTextDrawItem(
   const QMapLibreGL::CustomLayerRenderParameters&     params,
   const std::shared_ptr<gr::Placefile::TextDrawItem>& di)
{
   auto distance =
      (thresholded_) ?
         util::GeographicLib::GetDistance(
            params.latitude, params.longitude, di->latitude_, di->longitude_) :
         0;

   if (distance < di->threshold_)
   {
      const auto screenCoordinates = (util::maplibre::LatLongToScreenCoordinate(
                                         {di->latitude_, di->longitude_}) -
                                      mapScreenCoordLocation_) *
                                     mapScale_;

      RenderText(params,
                 di->text_,
                 di->hoverText_,
                 di->color_,
                 screenCoordinates.x + di->x_ + halfWidth_,
                 screenCoordinates.y + di->y_ + halfHeight_);
   }
}

void PlacefileLayer::Impl::RenderText(
   const QMapLibreGL::CustomLayerRenderParameters& params,
   const std::string&                              text,
   const std::string&                              hoverText,
   boost::gil::rgba8_pixel_t                       color,
   float                                           x,
   float                                           y)
{
   const std::string windowName {
      fmt::format("PlacefileText-{}-{}", placefileName_, ++textId_)};

   // Convert screen to ImGui coordinates
   y = params.height - y;

   // Setup "window" to hold text
   ImGui::SetNextWindowPos(
      ImVec2 {x, y}, ImGuiCond_Always, ImVec2 {0.5f, 0.5f});
   ImGui::Begin(windowName.c_str(),
                nullptr,
                ImGuiWindowFlags_AlwaysAutoResize |
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav |
                   ImGuiWindowFlags_NoBackground);

   // Render text
   ImGui::PushStyleColor(ImGuiCol_Text,
                         IM_COL32(color[0], color[1], color[2], color[3]));
   ImGui::TextUnformatted(text.c_str());
   ImGui::PopStyleColor();

   // Create tooltip for hover text
   if (!hoverText.empty() && ImGui::IsItemHovered())
   {
      ImGui::BeginTooltip();
      ImGui::PushFont(monospaceFont_);
      ImGui::TextUnformatted(hoverText.c_str());
      ImGui::PopFont();
      ImGui::EndTooltip();
   }

   // End window
   ImGui::End();
}

void PlacefileLayer::Render(
   const QMapLibreGL::CustomLayerRenderParameters& params)
{
   gl::OpenGLFunctions& gl = context()->gl();

   // Set OpenGL blend mode for transparency
   gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   // Reset text ID per frame
   p->textId_ = 0;

   // Update map screen coordinate and scale information
   p->mapScreenCoordLocation_ = util::maplibre::LatLongToScreenCoordinate(
      {params.latitude, params.longitude});
   p->mapScale_ = std::pow(2.0, params.zoom) * mbgl::util::tileSize_D /
                  mbgl::util::DEGREES_MAX;
   p->halfWidth_  = params.width * 0.5f;
   p->halfHeight_ = params.height * 0.5f;

   // Get monospace font pointer
   std::size_t fontSize = 16;
   auto        fontSizes =
      manager::SettingsManager::general_settings().font_sizes().GetValue();
   if (fontSizes.size() > 1)
   {
      fontSize = fontSizes[1];
   }
   else if (fontSizes.size() > 0)
   {
      fontSize = fontSizes[0];
   }
   auto monospace =
      manager::ResourceManager::Font(types::Font::Inconsolata_Regular);
   p->monospaceFont_ = monospace->ImGuiFont(fontSize);

   std::shared_ptr<manager::PlacefileManager> placefileManager =
      manager::PlacefileManager::Instance();

   auto placefile = placefileManager->placefile(p->placefileName_);

   // Render placefile
   if (placefile != nullptr)
   {
      p->thresholded_ =
         placefileManager->placefile_thresholded(placefile->name());

      if (p->dirty_)
      {
         // Reset Placefile Icons
         p->placefileIcons_->Reset();
         p->placefileIcons_->SetIconFiles(placefile->icon_files(),
                                          placefile->name());
      }

      for (auto& drawItem : placefile->GetDrawItems())
      {
         switch (drawItem->itemType_)
         {
         case gr::Placefile::ItemType::Text:
            p->RenderTextDrawItem(
               params,
               std::static_pointer_cast<gr::Placefile::TextDrawItem>(drawItem));
            break;

         case gr::Placefile::ItemType::Icon:
            p->RenderIconDrawItem(
               params,
               std::static_pointer_cast<gr::Placefile::IconDrawItem>(drawItem));
            break;

         default:
            break;
         }
      }
   }

   DrawLayer::Render(params);

   SCWX_GL_CHECK_ERROR();
}

void PlacefileLayer::Deinitialize()
{
   logger_->debug("Deinitialize()");

   DrawLayer::Deinitialize();
}

} // namespace map
} // namespace qt
} // namespace scwx
