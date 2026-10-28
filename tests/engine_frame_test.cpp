#include "frame_native.h"
#include <cmath>
#include <iostream>
void check(bool value){if(!value)throw std::runtime_error("Frame clock invariant failed");}
int main(){try{
 using namespace NativeEngine;
 bool rejected=false;try{BeginNativeFrame(0);}catch(const std::logic_error &){rejected=true;}check(rejected);
 InitializeFrameClock(1000);check(BeginNativeFrame(1016));
 check(Main::frameNumber==1&&Main::elapTime==16&&std::abs(Main::ElapTimeFrame()-0.016f)<0.000001f);
 SetFrameActive(FALSE,1020);check(!BeginNativeFrame(60000));check(Main::frameNumber==1);
 SetFrameActive(TRUE,60000);check(BeginNativeFrame(60020));check(Main::elapTime==20&&Main::frameNumber==2);
 // Repeated active notifications must not reset a frame's clock.
 SetFrameActive(TRUE,60025);check(BeginNativeFrame(60030)&&Main::elapTime==10);
 rejected=false;try{BeginNativeFrame(60029);}catch(const std::invalid_argument &){rejected=true;}check(rejected&&Main::thisTime==60030);
 InitializeFrameClock(0xfffffff0u);check(BeginNativeFrame(0x10)&&Main::elapTime==32);
 check(std::abs(Main::ElapTimeGame()-0.032f)<0.000001f);
 std::cout<<"Original frame state: timing, pause/resume, rollback rejection and U32 rollover passed; no game frame executed\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
