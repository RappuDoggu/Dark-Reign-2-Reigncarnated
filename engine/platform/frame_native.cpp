#include "frame_native.h"
#include "clock.h"
#include "crc.h"
#include <vector>
#include <algorithm>
namespace Main {
RunCodes runCodes("Main");
Bool active=FALSE;
U32 thisTime=0,elapTime=0,elapLast=0,frameNumber=0;
F32 elapSecs=0;
}
namespace {
U32 startTime=0;bool initialized=false;
struct Hook {U32 id;Main::GUIHookProc *proc;};
std::vector<Hook> hooks;
void DispatchActivation(Bool active){
 const U32 id=Crc::CalcStr("Activate");
 const auto snapshot=hooks;
 for(const auto &hook:snapshot)if(hook.id==id && std::any_of(hooks.begin(),hooks.end(),[&](const Hook &h){return h.id==id&&h.proc==hook.proc;}))hook.proc(id,0,std::uintptr_t(active),0);
}
}

namespace Main {
void RegisterGUIHook(const char *name,GUIHookProc *proc){
 if(!name||!proc)throw std::invalid_argument("Invalid GUI hook");
 const U32 id=Crc::CalcStr(name);
 for(const auto &hook:hooks)if(hook.id==id&&hook.proc==proc)throw std::logic_error("GUI hook already registered");
 hooks.push_back({id,proc});
}
void UnregisterGUIHook(const char *name,GUIHookProc *proc){
 if(!name||!proc)throw std::invalid_argument("Invalid GUI hook");
 const U32 id=Crc::CalcStr(name);
 const auto found=std::find_if(hooks.begin(),hooks.end(),[&](const Hook &hook){return hook.id==id&&hook.proc==proc;});
 if(found==hooks.end())throw std::logic_error("GUI hook not registered");
 hooks.erase(found);
}
F32 ElapTimeFrame(){return elapSecs;}
F32 ElapTimeGame(){return F32(thisTime-startTime)*0.001f;}
}
namespace NativeEngine {
void InitializeFrameClock(U32 now){
 startTime=Main::thisTime=now;
 Main::frameNumber=Main::elapTime=Main::elapLast=0;
 Main::elapSecs=0;Main::active=TRUE;initialized=true;
}
void SetFrameActive(Bool active,U32 now){
 if(!initialized)throw std::logic_error("Frame clock is not initialized");
 const bool changed=bool(Main::active)!=bool(active);
 if(changed){
  // Exclude the suspended interval from the next simulation delta.
  Main::thisTime=now;Main::elapTime=Main::elapLast=0;Main::elapSecs=0;
 }
 Main::active=active?TRUE:FALSE;
 if(changed)DispatchActivation(Main::active);
}
bool BeginNativeFrame(U32 now){
 if(!initialized)throw std::logic_error("Frame clock is not initialized");
 if(!Main::active)return false;
 const U32 delta=now-Main::thisTime;
 if(delta>0x7fffffffu)throw std::invalid_argument("Frame clock moved backwards");
 Main::thisTime=now;
 Main::elapLast=Main::elapTime=delta;Main::elapSecs=F32(delta)*0.001f;
 ++Main::frameNumber;return true;
}
bool BeginNativeFrame(){return BeginNativeFrame(Clock::Time::Ms());}
}
