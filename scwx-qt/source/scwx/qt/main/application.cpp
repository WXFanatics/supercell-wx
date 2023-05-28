#include <scwx/qt/main/application.hpp>
#include <scwx/util/logger.hpp>
#include <scwx/util/threads.hpp>

#include <condition_variable>

#include <boost/asio.hpp>

namespace scwx
{
namespace qt
{
namespace main
{
namespace Application
{

static const std::string logPrefix_ = "scwx::qt::main::application";
static const auto        logger_    = util::Logger::Create(logPrefix_);

// Initialization variables
static std::mutex              initializationMutex_ {};
static std::condition_variable initializationCondition_ {};
static bool                    initialized_ {false};

// Thread pool variables
static boost::asio::io_context& ioContext_ = scwx::util::io_context();
static auto work_ = boost::asio::make_work_guard(ioContext_);
static boost::asio::thread_pool threadPool_ {4};

void FinishInitialization()
{
   logger_->debug("Application initialization finished");

   // Set initialized to true
   std::unique_lock lock(initializationMutex_);
   initialized_ = true;
   lock.unlock();

   // Notify any threads waiting for initialization
   initializationCondition_.notify_all();
}

void WaitForInitialization()
{
   std::unique_lock lock(initializationMutex_);

   // While not yet initialized
   while (!initialized_)
   {
      // Wait for initialization
      initializationCondition_.wait(lock);
   }
}

void StartThreadPool()
{
   logger_->debug("Starting thread pool");

   // Start the io_context main loop
   boost::asio::post(threadPool_,
                     [&]()
                     {
                        while (true)
                        {
                           try
                           {
                              ioContext_.run();
                              break; // run() exited normally
                           }
                           catch (std::exception& ex)
                           {
                              // Log exception and continue
                              logger_->error(ex.what());
                           }
                        }
                     });
}

void StopThreadPool()
{
   logger_->debug("Stopping thread pool");

   // Gracefully stop the io_context main loop
   ioContext_.stop();
   work_.reset();
   threadPool_.join();

   logger_->debug("Thread pool stopped");
}

} // namespace Application
} // namespace main
} // namespace qt
} // namespace scwx
