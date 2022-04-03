#pragma once

#include <scwx/common/color_table.hpp>
#include <scwx/common/products.hpp>
#include <scwx/qt/manager/radar_product_manager.hpp>
#include <scwx/qt/view/radar_product_view.hpp>

#include <chrono>
#include <memory>
#include <vector>

namespace scwx
{
namespace qt
{
namespace view
{

class Level2ProductViewImpl;

class Level2ProductView : public RadarProductView
{
   Q_OBJECT

public:
   explicit Level2ProductView(
      common::Level2Product                         product,
      float                                         elevation,
      std::shared_ptr<manager::RadarProductManager> radarProductManager);
   ~Level2ProductView();

   const std::vector<boost::gil::rgba8_pixel_t>& color_table() const override;
   uint16_t                              color_table_min() const override;
   uint16_t                              color_table_max() const override;
   float                                 elevation() const override;
   float                                 range() const override;
   std::chrono::system_clock::time_point sweep_time() const override;
   uint16_t                              vcp() const override;
   const std::vector<float>&             vertices() const override;

   void LoadColorTable(std::shared_ptr<common::ColorTable> colorTable) override;
   void SelectElevation(float elevation) override;
   void SelectTime(std::chrono::system_clock::time_point time) override;
   void Update() override;

   common::RadarProductGroup GetRadarProductGroup() const override;
   std::string               GetRadarProductName() const override;
   std::vector<float>        GetElevationCuts() const override;
   std::tuple<const void*, size_t, size_t> GetMomentData() const override;
   std::tuple<const void*, size_t, size_t> GetCfpMomentData() const override;

   size_t
   GlBufferVertices(gl::OpenGLFunctions& gl, GLuint vbo, GLuint index) override;
   void GlBufferMomentData(gl::OpenGLFunctions& gl,
                           GLuint               vbo,
                           GLuint               index) override;
   bool GlBufferCfpMomentData(gl::OpenGLFunctions& gl,
                              GLuint               vbo,
                              GLuint               index) override;

   static std::shared_ptr<Level2ProductView>
   Create(common::Level2Product                         product,
          float                                         elevation,
          std::shared_ptr<manager::RadarProductManager> radarProductManager);

protected:
   void UpdateColorTable() override;

protected slots:
   void ComputeSweep() override;

private:
   std::unique_ptr<Level2ProductViewImpl> p;
};

} // namespace view
} // namespace qt
} // namespace scwx
