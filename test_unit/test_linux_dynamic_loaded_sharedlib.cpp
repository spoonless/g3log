/** ==========================================================================
* 2014 by KjellKod.cc. This is PUBLIC DOMAIN to use at your own risk and comes
* with no warranties. This code is yours to share, use and modify with no
* strings attached and no restrictions or obligations.
 * 
 * For more information see g3log/LICENSE or refer refer to http://unlicense.org
* ============================================================================*/


#include <g2log.hpp>
#include <g2logworker.hpp>
#include <g2filesink.hpp>
#include <std2_make_unique.hpp>

#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "tester_sharedlib.h"
#include <dlfcn.h>

struct LogMessageCounter {
   std::vector<std::string>& bank;
   LogMessageCounter(std::vector<std::string>& storeMessages) : bank(storeMessages) {
   }

   void countMessages(std::string msg) {
      bank.push_back(msg);
   }
};

TEST(DynamicLoadOfLibrary, JustLoadAndExit) {
   std::vector<std::string> receiver;
   
   { // scope to flush logs at logworker exit
      auto worker = g2::LogWorker::createWithNoSink();
      auto handle = worker->addSink(std2::make_unique<LogMessageCounter>(std::ref(receiver)), &LogMessageCounter::countMessages);
      
      // add another sink just for more throughput of data
      auto fileHandle = worker->addSink(std2::make_unique<g2::FileSink>("runtimeLoadOfDynamiclibs", "/tmp"), &g2::FileSink::fileWrite);
      g2::initializeLogging(worker.get());

      void* libHandle = dlopen("libtester_sharedlib.so", RTLD_LAZY | RTLD_GLOBAL);
      EXPECT_FALSE(nullptr == libHandle);
      LibraryFactory* factory = reinterpret_cast<LibraryFactory*> ((dlsym(libHandle, "testRealFactory")));
      EXPECT_FALSE(nullptr == factory);
      SomeLibrary* loadedLibrary = factory->CreateLibrary();

      for (size_t i = 0; i < 300; ++i) {
         loadedLibrary->action();
      }

      delete loadedLibrary;
      dlclose(libHandle);
   } // scope exit. All log entries must be flushed now
   const int numberOfMessages = 2 + 300 + 1; // 2 library construction, 300 loop, 1 destoyed library  
   EXPECT_EQ(receiver.size(), numberOfMessages);
}
