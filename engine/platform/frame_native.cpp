#include "frame_native.h"
#include "clock.h"
namespace Main {
RunCodes runCodes("Main");
Bool active=FALSE;
U32 thisTime=0,elapTime=0,elapLast=0,frameNumber=0;
F32 elapSecs=0;
}
namespace { U32 startTime=0;bool initialized=false; }
namespace Main {
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
 if(bool(Main::active)!=bool(active)){
  // Exclude the suspended interval from the next simulation delta.
  Main::thisTime=now;Main::elapTime=Main::elapLast=0;Main::elapSecs=0;
 }
 Main::active=active?TRUE:FALSE;
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
