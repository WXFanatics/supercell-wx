#include <scwx/qt/config/radar_site.hpp>
#include <scwx/qt/main/application.hpp>
#include <scwx/qt/main/main_window.hpp>
#include <scwx/qt/manager/radar_product_manager.hpp>
#include <scwx/qt/manager/resource_manager.hpp>
#include <scwx/qt/manager/settings_manager.hpp>
#include <scwx/util/logger.hpp>

#include <aws/core/Aws.h>
#include <spdlog/spdlog.h>
#include <QApplication>
#include <QTranslator>

static const std::string logPrefix_ = "scwx::main";
static const auto        logger_    = scwx::util::Logger::Create(logPrefix_);

int main(int argc, char* argv[])
{
   // Initialize logger
   scwx::util::Logger::Initialize();
   spdlog::set_level(spdlog::level::debug);

   QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);

   QApplication a(argc, argv);

   QCoreApplication::setApplicationName("Supercell Wx");

   // Enable internationalization support
   QTranslator translator;
   if (translator.load(QLocale(), "scwx", "_", ":/i18n"))
   {
      QCoreApplication::installTranslator(&translator);
   }

   // Start the io_context main loop
   scwx::qt::main::Application::StartThreadPool();

   // Initialize AWS SDK
   Aws::SDKOptions awsSdkOptions;
   Aws::InitAPI(awsSdkOptions);

   // Initialize application
   scwx::qt::config::RadarSite::Initialize();
   scwx::qt::manager::SettingsManager::Initialize();
   scwx::qt::manager::ResourceManager::Initialize();

   // Run Qt main loop
   int result;
   {
      scwx::qt::main::MainWindow w;
      w.show();
      result = a.exec();
   }

   // Deinitialize application
   scwx::qt::manager::RadarProductManager::Cleanup();

   // Shutdown application
   scwx::qt::manager::ResourceManager::Shutdown();

   // Shutdown AWS SDK
   Aws::ShutdownAPI(awsSdkOptions);

   return result;
}
