#include "iface_events.hpp"
#include "reign/interface_events.hpp"
#include <iostream>
#include <vector>
#include "reign/event_loop.hpp"
int main(){try{
 unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::logic_error("IFace event check "+std::to_string(checks));};
 reign::InterfaceEvents first,second;
 first.run([&]{
  check(IFace::EventID()!=0&&IFace::GetFocus()==nullptr&&IFace::GetCapture()==nullptr&&IFace::GetModal()==nullptr);
  check(IFace::SetFlag(IFace::DISABLE_DRAW,TRUE)==FALSE);check(IFace::GetFlag(IFace::DISABLE_DRAW));
  second.run([&]{check(!IFace::GetFlag(IFace::DISABLE_DRAW));});check(IFace::GetFlag(IFace::DISABLE_DRAW));
  try{second.run([]{throw std::runtime_error("injected");});}catch(const std::runtime_error&){}
  check(IFace::GetFlag(IFace::DISABLE_DRAW));
  IFace::SetFocus(nullptr);IFace::SetMouseCapture(nullptr);IFace::SetTipControl(nullptr);
  check(!IFace::SendNotifyFocus(nullptr,1)&&!IFace::PostNotifyModal(nullptr,1));
  bool rejected=false;try{IFace::SendEvent(nullptr,nullptr,0);}catch(const std::runtime_error&){rejected=true;}check(rejected);
  // Original null-recipient Reaper path: queue, detect dead target, dispose.
  for(unsigned i=0;i<128;++i)check(IFace::PostNotify(nullptr,nullptr,i)!=0);
  check(!IFace::PostNotify(nullptr,nullptr,129));
 });
 check(!first.empty()&&second.empty());first.process();check(!first.empty());first.process();check(first.empty());
 bool unbound=false;try{IFace::EventID();}catch(const std::runtime_error&){unbound=true;}check(unbound);
 {reign::InterfaceEvents pending;pending.run([&]{for(unsigned i=0;i<12;++i)check(IFace::PostNotify(nullptr,nullptr,i)!=0);});}
 // A malformed event forces the original handler assertion, leaving the rest
 // of its work batch pending. Session destruction must free those Reapers too.
 {reign::InterfaceEvents interrupted;
  interrupted.run([&]{Event malformed{};malformed.type=IFace::EventID();check(EventSys::Put(malformed));
   for(unsigned i=0;i<12;++i)check(IFace::PostNotify(nullptr,nullptr,i)!=0);
  });
  bool caught=false;try{interrupted.process();}catch(const std::runtime_error&){caught=true;}
  check(caught&&!interrupted.empty());
 }
 // Borrow the engine queue rather than changing input/interface event ordering.
 {reign::EventLoop shared;const auto id=shared.register_type("OTHER");std::vector<U32> values;
  shared.set_handler(id,[&](const auto &e){values.push_back(e.parameters[0]);return false;});
  {reign::InterfaceEvents ui(shared);
   ui.run([&]{check(IFace::PostNotify(nullptr,nullptr,6));});shared.process();check(ui.empty());
   reign::EngineEvent e;e.type=id;e.parameters[0]=11;check(shared.put(e));
   ui.run([&]{check(IFace::PostNotify(nullptr,nullptr,7));});e.parameters[0]=22;check(shared.put(e));
  }
  shared.process();check(values==std::vector<U32>({11,22}));
  {reign::InterfaceEvents ui(shared);reign::EngineEvent e;e.type=id;e.parameters[0]=33;check(shared.put(e));
   ui.run([&]{check(IFace::PostNotify(nullptr,nullptr,8));});e.parameters[0]=44;check(shared.put(e));
   shared.set_handler(id,[](const auto &){throw std::runtime_error("interrupted work batch");return false;});
   bool caught=false;try{shared.process();}catch(const std::runtime_error&){caught=true;}check(caught);
  }
  shared.set_handler(id,[&](const auto &e){values.push_back(e.parameters[0]);return false;});
  shared.process();check(values==std::vector<U32>({11,22,44}));
 }
 std::cout<<"Original IFace flags/context/notify/dead-recipient/queue saturation/shutdown: PASS ("<<checks<<" checks)\n";
 std::cout<<"Live IControl focus/modal/render tree: not exercised\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
