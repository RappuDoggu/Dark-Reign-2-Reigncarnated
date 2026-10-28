#include "debug_native.h"
#include "native_stack.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <stdexcept>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
namespace {
struct ErrorContext { std::string file, timestamp; std::uint32_t line=0; Debug::Error::Type type=Debug::Error::FATAL; };
thread_local ErrorContext context;
class Watchdog {
 std::mutex mutex;
 std::condition_variable changed;
 std::thread worker;
 bool enabled=false,stop=false;
 using Clock=std::chrono::steady_clock;
 Clock::time_point heartbeat;
public:
 ~Watchdog(){finish();}
 void enable(){std::lock_guard<std::mutex> lock(mutex);enabled=true;}
 void start(){
  std::lock_guard<std::mutex> lock(mutex);
  if(!enabled||worker.joinable())return;
  stop=false;heartbeat=Clock::now();
  worker=std::thread([this]{
   std::unique_lock<std::mutex> lock(mutex);
   while(!stop){
    const auto deadline=heartbeat+std::chrono::minutes(4);
    if(changed.wait_until(lock,deadline,[&]{return stop||heartbeat+std::chrono::minutes(4)!=deadline;}))continue;
    std::fputs("Native engine watchdog: no main-thread heartbeat for 240 seconds\n",stderr);
    // Original policy: warn after four minutes, fail after one more minute.
    if(!changed.wait_for(lock,std::chrono::seconds(60),[&]{return stop||heartbeat+std::chrono::minutes(4)!=deadline;})){
     std::fputs("Native engine watchdog: main thread remains unresponsive; aborting\n",stderr);std::abort();
    }
   }
  });
 }
 void poll(){std::lock_guard<std::mutex> lock(mutex);heartbeat=Clock::now();changed.notify_all();}
 void finish(){
  {std::lock_guard<std::mutex> lock(mutex);stop=true;changed.notify_all();}
  if(worker.joinable())worker.join();
 }
};
Watchdog &watchdog(){static Watchdog instance;return instance;}
}
namespace Debug {
namespace Error {
void Set(const char *file,std::uint32_t line,const char *timestamp,Type type){context={file?file:"",timestamp?timestamp:"",line,type};}
[[noreturn]] void Err(const char *format,...){
 char message[4096];va_list args;va_start(args,format);
 std::vsnprintf(message,sizeof(message),format?format:"Engine error",args);va_end(args);
 const std::string detail=context.file+":"+std::to_string(context.line)+" ["+std::to_string(int(context.type))+"] "+message;
 std::fprintf(stderr,"%s\n",detail.c_str());
 CallStack::Dump();
 throw std::runtime_error(detail);
}
}
namespace CallStack {
void Dump(){std::uintptr_t frames[64];const auto count=NativeEngine::CaptureStack(frames,64);
 for(std::size_t i=0;i<count;++i)std::fprintf(stderr,"  #%zu %p\n",i,reinterpret_cast<void *>(frames[i]));
}
}
namespace Watchdog {
void Enable(){watchdog().enable();}
void Create(){watchdog().start();}
void Delete(){watchdog().finish();}
void Poll(){watchdog().poll();}
}
}
