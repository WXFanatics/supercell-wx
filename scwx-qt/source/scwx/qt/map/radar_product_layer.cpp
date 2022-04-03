#include <scwx/qt/map/radar_product_layer.hpp>
#include <scwx/qt/gl/shader_program.hpp>

#include <execution>

#include <boost/log/trivial.hpp>
#include <boost/timer/timer.hpp>
#include <GeographicLib/Geodesic.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <mbgl/util/constants.hpp>

namespace scwx
{
namespace qt
{
namespace map
{

static constexpr uint32_t MAX_RADIALS           = 720;
static constexpr uint32_t MAX_DATA_MOMENT_GATES = 1840;

static const std::string logPrefix_ = "[scwx::qt::map::radar_product_layer] ";

static glm::vec2
LatLongToScreenCoordinate(const QMapbox::Coordinate& coordinate);

class RadarProductLayerImpl
{
public:
   explicit RadarProductLayerImpl(std::shared_ptr<MapContext> context) :
       shaderProgram_(context->gl_),
       uMVPMatrixLocation_(GL_INVALID_INDEX),
       uMapScreenCoordLocation_(GL_INVALID_INDEX),
       uDataMomentOffsetLocation_(GL_INVALID_INDEX),
       uDataMomentScaleLocation_(GL_INVALID_INDEX),
       uCFPEnabledLocation_(GL_INVALID_INDEX),
       vbo_ {GL_INVALID_INDEX},
       vao_ {GL_INVALID_INDEX},
       texture_ {GL_INVALID_INDEX},
       numVertices_ {0},
       cfpEnabled_ {false},
       colorTableNeedsUpdate_ {false},
       sweepNeedsUpdate_ {false}
   {
   }
   ~RadarProductLayerImpl() = default;

   gl::ShaderProgram     shaderProgram_;
   GLint                 uMVPMatrixLocation_;
   GLint                 uMapScreenCoordLocation_;
   GLint                 uDataMomentOffsetLocation_;
   GLint                 uDataMomentScaleLocation_;
   GLint                 uCFPEnabledLocation_;
   std::array<GLuint, 3> vbo_;
   GLuint                vao_;
   GLuint                texture_;

   GLsizeiptr numVertices_;

   bool cfpEnabled_;

   bool colorTableNeedsUpdate_;
   bool sweepNeedsUpdate_;
};

RadarProductLayer::RadarProductLayer(std::shared_ptr<MapContext> context) :
    GenericLayer(context), p(std::make_unique<RadarProductLayerImpl>(context))
{
}
RadarProductLayer::~RadarProductLayer() = default;

void RadarProductLayer::Initialize()
{
   BOOST_LOG_TRIVIAL(debug) << logPrefix_ << "Initialize()";

   gl::OpenGLFunctions& gl = context()->gl_;

   // Load and configure radar shader
   p->shaderProgram_.Load(":/gl/radar.vert", ":/gl/radar.frag");

   p->uMVPMatrixLocation_ =
      gl.glGetUniformLocation(p->shaderProgram_.id(), "uMVPMatrix");
   if (p->uMVPMatrixLocation_ == -1)
   {
      BOOST_LOG_TRIVIAL(warning) << logPrefix_ << "Could not find uMVPMatrix";
   }

   p->uMapScreenCoordLocation_ =
      gl.glGetUniformLocation(p->shaderProgram_.id(), "uMapScreenCoord");
   if (p->uMapScreenCoordLocation_ == -1)
   {
      BOOST_LOG_TRIVIAL(warning)
         << logPrefix_ << "Could not find uMapScreenCoord";
   }

   p->uDataMomentOffsetLocation_ =
      gl.glGetUniformLocation(p->shaderProgram_.id(), "uDataMomentOffset");
   if (p->uDataMomentOffsetLocation_ == -1)
   {
      BOOST_LOG_TRIVIAL(warning)
         << logPrefix_ << "Could not find uDataMomentOffset";
   }

   p->uDataMomentScaleLocation_ =
      gl.glGetUniformLocation(p->shaderProgram_.id(), "uDataMomentScale");
   if (p->uDataMomentScaleLocation_ == -1)
   {
      BOOST_LOG_TRIVIAL(warning)
         << logPrefix_ << "Could not find uDataMomentScale";
   }

   p->uCFPEnabledLocation_ =
      gl.glGetUniformLocation(p->shaderProgram_.id(), "uCFPEnabled");
   if (p->uCFPEnabledLocation_ == -1)
   {
      BOOST_LOG_TRIVIAL(warning) << logPrefix_ << "Could not find uCFPEnabled";
   }

   p->shaderProgram_.Use();

   // Generate a vertex array object
   gl.glGenVertexArrays(1, &p->vao_);

   // Generate vertex buffer objects
   gl.glGenBuffers(3, p->vbo_.data());

   // Update radar sweep
   UpdateSweep();

   // Create color table
   gl.glGenTextures(1, &p->texture_);
   UpdateColorTable();
   gl.glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

   connect(context()->radarProductView_.get(),
           &view::RadarProductView::ColorTableUpdated,
           this,
           [=]() { p->colorTableNeedsUpdate_ = true; });
   connect(context()->radarProductView_.get(),
           &view::RadarProductView::SweepComputed,
           this,
           [=]() { p->sweepNeedsUpdate_ = true; });
}

void RadarProductLayer::UpdateSweep()
{
   BOOST_LOG_TRIVIAL(debug) << logPrefix_ << "UpdateSweep()";

   p->sweepNeedsUpdate_ = false;

   gl::OpenGLFunctions& gl = context()->gl_;

   boost::timer::cpu_timer timer;

   std::shared_ptr<view::RadarProductView> radarProductView =
      context()->radarProductView_;

   // Bind a vertex array object
   gl.glBindVertexArray(p->vao_);

   // Buffer vertices
   timer.start();
   p->numVertices_ = radarProductView->GlBufferVertices(gl, p->vbo_[0], 0);
   timer.stop();
   gl.glEnableVertexAttribArray(0);

   BOOST_LOG_TRIVIAL(debug)
      << logPrefix_ << "Vertices buffered in " << timer.format(6, "%ws");

   // Buffer data moments
   timer.start();
   radarProductView->GlBufferMomentData(gl, p->vbo_[1], 1);
   timer.stop();
   gl.glEnableVertexAttribArray(1);

   BOOST_LOG_TRIVIAL(debug)
      << logPrefix_ << "Data moments buffered in " << timer.format(6, "%ws");

   // Buffer CFP data
   timer.start();
   bool cfpPresent = radarProductView->GlBufferCfpMomentData(gl, p->vbo_[2], 2);
   timer.stop();

   if (cfpPresent)
   {
      BOOST_LOG_TRIVIAL(debug)
         << logPrefix_ << "CFP moments buffered in " << timer.format(6, "%ws");

      gl.glEnableVertexAttribArray(2);
   }
   else
   {
      gl.glDisableVertexAttribArray(2);
   }
}

void RadarProductLayer::Render(
   const QMapbox::CustomLayerRenderParameters& params)
{
   gl::OpenGLFunctions& gl = context()->gl_;

   p->shaderProgram_.Use();

   if (p->colorTableNeedsUpdate_)
   {
      UpdateColorTable();
   }

   if (p->sweepNeedsUpdate_)
   {
      UpdateSweep();
   }

   const float scale = std::pow(2.0, params.zoom) * 2.0f *
                       mbgl::util::tileSize / mbgl::util::DEGREES_MAX;
   const float xScale = scale / params.width;
   const float yScale = scale / params.height;

   glm::mat4 uMVPMatrix(1.0f);
   uMVPMatrix = glm::scale(uMVPMatrix, glm::vec3(xScale, yScale, 1.0f));
   uMVPMatrix = glm::rotate(uMVPMatrix,
                            glm::radians<float>(params.bearing),
                            glm::vec3(0.0f, 0.0f, 1.0f));

   gl.glUniform2fv(p->uMapScreenCoordLocation_,
                   1,
                   glm::value_ptr(LatLongToScreenCoordinate(
                      {params.latitude, params.longitude})));

   gl.glUniformMatrix4fv(
      p->uMVPMatrixLocation_, 1, GL_FALSE, glm::value_ptr(uMVPMatrix));

   gl.glUniform1i(p->uCFPEnabledLocation_, p->cfpEnabled_ ? 1 : 0);

   gl.glActiveTexture(GL_TEXTURE0);
   gl.glBindTexture(GL_TEXTURE_1D, p->texture_);
   gl.glBindVertexArray(p->vao_);
   gl.glDrawArrays(GL_TRIANGLES, 0, p->numVertices_);
}

void RadarProductLayer::Deinitialize()
{
   BOOST_LOG_TRIVIAL(debug) << logPrefix_ << "Deinitialize()";

   gl::OpenGLFunctions& gl = context()->gl_;

   gl.glDeleteVertexArrays(1, &p->vao_);
   gl.glDeleteBuffers(3, p->vbo_.data());

   p->uMVPMatrixLocation_        = GL_INVALID_INDEX;
   p->uMapScreenCoordLocation_   = GL_INVALID_INDEX;
   p->uDataMomentOffsetLocation_ = GL_INVALID_INDEX;
   p->uDataMomentScaleLocation_  = GL_INVALID_INDEX;
   p->uCFPEnabledLocation_       = GL_INVALID_INDEX;
   p->vao_                       = GL_INVALID_INDEX;
   p->vbo_                       = {GL_INVALID_INDEX};
   p->texture_                   = GL_INVALID_INDEX;
}

void RadarProductLayer::UpdateColorTable()
{
   BOOST_LOG_TRIVIAL(debug) << logPrefix_ << "UpdateColorTable()";

   p->colorTableNeedsUpdate_ = false;

   gl::OpenGLFunctions&                    gl = context()->gl_;
   std::shared_ptr<view::RadarProductView> radarProductView =
      context()->radarProductView_;

   const std::vector<boost::gil::rgba8_pixel_t>& colorTable =
      radarProductView->color_table();
   const uint16_t rangeMin = radarProductView->color_table_min();
   const uint16_t rangeMax = radarProductView->color_table_max();

   const float scale = rangeMax - rangeMin;

   gl.glActiveTexture(GL_TEXTURE0);
   gl.glBindTexture(GL_TEXTURE_1D, p->texture_);
   gl.glTexImage1D(GL_TEXTURE_1D,
                   0,
                   GL_RGBA,
                   (GLsizei) colorTable.size(),
                   0,
                   GL_RGBA,
                   GL_UNSIGNED_BYTE,
                   colorTable.data());
   gl.glGenerateMipmap(GL_TEXTURE_1D);

   gl.glUniform1ui(p->uDataMomentOffsetLocation_, rangeMin);
   gl.glUniform1f(p->uDataMomentScaleLocation_, scale);
}

static glm::vec2
LatLongToScreenCoordinate(const QMapbox::Coordinate& coordinate)
{
   double latitude = std::clamp(
      coordinate.first, -mbgl::util::LATITUDE_MAX, mbgl::util::LATITUDE_MAX);
   glm::vec2 screen {
      mbgl::util::LONGITUDE_MAX + coordinate.second,
      -(mbgl::util::LONGITUDE_MAX -
        mbgl::util::RAD2DEG *
           std::log(std::tan(M_PI / 4.0 +
                             latitude * M_PI / mbgl::util::DEGREES_MAX)))};
   return screen;
}

} // namespace map
} // namespace qt
} // namespace scwx
