#include "mission_prelude.h"
#include "type_ident_registry.h"
#include "stdload.h"
#include "system_sync.h"
#include "native_stack.h"
#include <thread>
#include <iostream>
#include <vector>
#include <atomic>
static void check(bool value) { if(!value) throw std::runtime_error("Native runtime invariant"); }
int main() {
 try {
  using namespace NativeEngine;
  struct Target {};
  FScope config(nullptr,"Type");config.AddArgString("original.loader.type");config.InitIterators();
  Reaper<Target> loaded;
  StdLoad::TypeReaperObjType(&config,loaded);
  check(loaded.HasResolveId());
  check(std::strcmp(TakeTypeIdent(loaded.GetResolveId()).str,"original.loader.type")==0);
  auto first=StoreTypeIdent("sprawler.constructor");
  auto second=StoreTypeIdent("jda.hq");
  check(first!=second && PendingTypeIdents()==2);
  check(std::strcmp(TakeTypeIdent(first).str,"sprawler.constructor")==0);
  bool rejected=false;try { TakeTypeIdent(first); } catch(const std::logic_error &) { rejected=true; }
  check(rejected);
  ClearTypeIdents();
  rejected=false;try { TakeTypeIdent(second); } catch(const std::logic_error &) { rejected=true; }
  check(rejected && PendingTypeIdents()==0);
  check(StoreTypeIdent("next.mission")>second);ClearTypeIdents();
  std::vector<std::thread> workers;
  for(int i=0;i<4;++i)workers.emplace_back([] {
   for(int n=0;n<500;++n) {
    auto token=StoreTypeIdent("concurrent.type");
    check(std::strcmp(TakeTypeIdent(token).str,"concurrent.type")==0);
   }
  });
  for(auto &worker:workers)worker.join();
  check(PendingTypeIdents()==0);
  System::Semaphore ready(0,1), consumed(0,1);
  System::Mutex mutex;
  int value=0;
  check(!ready.Wait(0));
  std::thread consumer([&] { check(ready.Wait(2000));check(mutex.Wait());check(value==42);mutex.Signal();consumed.Signal(); });
  check(mutex.Wait());value=42;mutex.Signal();ready.Signal();
  check(consumed.Wait(2000));consumer.join();check(!ready.Test());
  ready.Signal();rejected=false;
  try { ready.Signal(); } catch(const std::overflow_error &) { rejected=true; }
  check(rejected && ready.Wait(0));
  check(mutex.Wait() && mutex.Wait(0));mutex.Signal();mutex.Signal();
  std::uintptr_t frames[32];check(CaptureStack(frames,32)>0);
  std::cout<<"Type identifiers, concurrent ownership, native queues and unwinding passed\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
