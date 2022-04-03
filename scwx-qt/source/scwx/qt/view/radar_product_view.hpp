#pragma once

#include <scwx/common/color_table.hpp>
#include <scwx/common/products.hpp>
#include <scwx/qt/gl/gl.hpp>

#include <chrono>
#include <memory>
#include <vector>

#include <QObject>

namespace scwx
{
namespace qt
{
namespace view
{

class RadarProductViewImpl;

class RadarProductView : public QObject
{
   Q_OBJECT

public:
   explicit RadarProductView();
   virtual ~RadarProductView();

   virtual const std::vector<boost::gil::rgba8_pixel_t>& color_table() const;
   virtual uint16_t                              color_table_min() const;
   virtual uint16_t                              color_table_max() const;
   virtual float                                 elevation() const;
   virtual float                                 range() const;
   virtual std::chrono::system_clock::time_point sweep_time() const;
   virtual uint16_t                              vcp() const      = 0;
   virtual const std::vector<float>&             vertices() const = 0;

   void Initialize();
   virtual void
   LoadColorTable(std::shared_ptr<common::ColorTable> colorTable) = 0;
   virtual void SelectElevation(float elevation);
   virtual void SelectTime(std::chrono::system_clock::time_point time) = 0;
   virtual void Update()                                               = 0;

   virtual common::RadarProductGroup GetRadarProductGroup() const = 0;
   virtual std::string               GetRadarProductName() const  = 0;
   virtual std::vector<float>        GetElevationCuts() const;
   virtual std::tuple<const void*, size_t, size_t> GetMomentData() const = 0;
   virtual std::tuple<const void*, size_t, size_t> GetCfpMomentData() const;

   virtual size_t
   GlBufferVertices(gl::OpenGLFunctions& gl, GLuint vbo, GLuint index) = 0;
   virtual void
   GlBufferMomentData(gl::OpenGLFunctions& gl, GLuint vbo, GLuint index) = 0;
   virtual bool
   GlBufferCfpMomentData(gl::OpenGLFunctions& gl, GLuint vbo, GLuint index);

protected:
   virtual void UpdateColorTable() = 0;

protected slots:
   virtual void ComputeSweep();

signals:
   void ColorTableUpdated();
   void SweepComputed();

private:
   std::unique_ptr<RadarProductViewImpl> p;
};

} // namespace view
} // namespace qt
} // namespace scwx
