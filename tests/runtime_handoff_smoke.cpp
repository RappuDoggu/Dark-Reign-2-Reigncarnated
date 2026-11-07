#include "varsys.h"
#include "reign/event_loop.hpp"
#include <iostream>
using namespace reign::vars_detail;
namespace {unsigned commandCount=0;void command(U32){++commandCount;}}
struct Target {DTrack::Info dTrack;VarInteger value;};
int main(){try{
 TrackSys::Init();VarSys::Init();unsigned changes=0;
 VarSys::RegisterHandler("runtime",command);VarSys::CreateCmd("runtime.command",VarSys::NOTIFY);
 Target target;TrackSys::RegisterConstruction(target.dTrack);Reaper<Target> observer(&target);
 VarSys::CreateInteger("runtime.value",0,0,&target.value);VarSys::SetIntegerRange("runtime.value",0,10);
 {reign::EventLoop loop;auto iface=loop.register_type("IFACE",reign::EventPayload::interface),cmd=loop.register_type("COMMAND");
 loop.set_handler(iface,[&](const auto &e){if(e.to!=&target||!observer.Alive())throw std::runtime_error("Invalid tracked target");target.value=e.interface_parameters[0];++changes;reign::EngineEvent next;next.type=cmd;loop.put(next);return true;});
 loop.set_handler(cmd,[&](const auto &){return VarSys::TriggerCmd("runtime.command");});
 reign::EngineEvent e;e.type=iface;e.to=&target;e.from=&loop;e.interface_parameters[0]=50;
 if(!loop.put(e))throw std::runtime_error("Event enqueue failed");loop.process();
 if(*target.value!=10||changes!=1||commandCount!=0||loop.empty())throw std::runtime_error("Original first-frame event/variable handoff differs");
 loop.process();if(commandCount!=1||!loop.empty())throw std::runtime_error("Original deferred command handoff differs");
 }
 VarSys::DeleteItem("runtime");if(target.value.Alive())throw std::runtime_error("Variable death tracking failed");TrackSys::RegisterDestruction(target.dTrack);if(observer.Alive())throw std::runtime_error("Tracked target remained alive");VarSys::Done();TrackSys::Done();
 std::cout<<"Original EventSys -> tracked native pointer -> VarSys clamp -> deferred command: PASS\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
