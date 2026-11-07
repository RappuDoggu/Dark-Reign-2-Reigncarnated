#pragma once
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#ifndef INFINITE
#define INFINITE 0xffffffffu
#endif
// Process-local synchronization used by original order queues. Named Windows
// kernel objects require a separate backend and fail explicitly here.
class System {
public:
 class Thread;
 class Event;
 class Mutex {
  std::recursive_timed_mutex mutex;
 public:
  explicit Mutex(const char *name=nullptr) { if(name) throw std::runtime_error("Named mutex unavailable"); }
  bool Wait(U32 timeout=INFINITE) {
   if(timeout==INFINITE) { mutex.lock(); return true; }
   return mutex.try_lock_for(std::chrono::milliseconds(timeout));
  }
  void Signal() { mutex.unlock(); }
  bool IsPrimary() const { return true; }
 };
 class Semaphore {
  std::mutex mutex;
  std::condition_variable changed;
  int count,maximum;
 public:
  Semaphore(int initial,int max,const char *name=nullptr):count(initial),maximum(max) {
   if(name) throw std::runtime_error("Named semaphore unavailable");
   if(initial<0 || max<=0 || initial>max) throw std::invalid_argument("Semaphore bounds");
  }
  bool Wait(U32 timeout=INFINITE) {
   std::unique_lock<std::mutex> lock(mutex);
   if(timeout==INFINITE) changed.wait(lock,[this]{return count>0;});
   else if(!changed.wait_for(lock,std::chrono::milliseconds(timeout),[this]{return count>0;})) return false;
   --count; return true;
  }
  bool Test() { std::lock_guard<std::mutex> lock(mutex); return count>0; }
  void Signal(int step=1) {
   { std::lock_guard<std::mutex> lock(mutex);
     if(step<=0 || step>maximum-count) throw std::overflow_error("Semaphore release bounds");
     count+=step;
   }
   changed.notify_all();
  }
  bool IsPrimary() const { return true; }
 };
};
