#include "input_native.h"
#include "event.h"
#include <stdexcept>
#include <iostream>
void check(bool value){if(!value)throw std::runtime_error("Input invariant failed");}
int main(){
 try{
  EventSys::Init();Input::Init(nullptr,nullptr);NativeEngine::SetInputViewport(800,600);
  U16 other;EventSys::RegisterEvent("OTHER",other);
  NativeEngine::QueuePointer(900,-5,0,TRUE);NativeEngine::QueuePointer(300,250,0,FALSE);Input::ReadEvents();
  Event e;check(EventSys::Get(e));check(e.type==Input::eventId&&e.subType==Input::MOUSEBUTTONDOWN&&e.input.code==0);
  check(e.input.mouseX==799&&e.input.mouseY==0&&(e.input.state&Input::LBUTTONDOWN));
  check(EventSys::Get(e));check(e.subType==Input::MOUSEBUTTONUP&&e.input.mouseX==300&&!(e.input.state&Input::LBUTTONDOWN));
  NativeEngine::QueueKey(0x2a,TRUE);NativeEngine::QueueKey(0x1e,TRUE,'A');Input::ReadEvents();
  check(Input::IsKeyDown(0x2a)&&Input::KeyState(Input::SHIFTDOWN));
  check(EventSys::Get(e)&&e.subType==Input::KEYDOWN);check(EventSys::Get(e)&&e.subType==Input::KEYDOWN);
  check(EventSys::Get(e)&&e.subType==Input::KEYCHAR&&e.input.ch=='A');
  Event preserved{};preserved.type=other;preserved.param1=123;check(EventSys::Put(preserved));
  NativeEngine::QueuePointer(7,8,0,TRUE);Input::ReadEvents();Input::OnActivate(FALSE);
  check(!Input::KeyState(Input::SHIFTDOWN)&&!Input::IsKeyDown(0x1e));
  check(EventSys::Get(e)&&e.type==other&&e.param1==123);check(!EventSys::Get(e));
  Input::ReadEvents();unsigned releases=0;while(EventSys::Get(e)){check(e.subType==Input::KEYUP||e.subType==Input::MOUSEBUTTONUP);++releases;}check(releases==3);
  Input::OnActivate(TRUE);
  NativeEngine::QueueWheel(-120);Input::ReadEvents();check(EventSys::Get(e)&&e.subType==Input::MOUSEAXIS&&e.input.code==0&&e.input.ch==-120);
  for(int i=0;i<128;++i)check(EventSys::Put(preserved));check(EventSys::QueueFull());check(!EventSys::QueueEmpty());
  NativeEngine::QueuePointer(12,34,-1,FALSE);Input::ReadEvents();
  unsigned count=0;while(EventSys::Get(e)){check(e.type==other);++count;}check(count==128);
  Input::ReadEvents();check(EventSys::Get(e)&&e.subType==Input::MOUSEMOVE&&e.input.mouseX==12);check(!EventSys::Get(e));
  check(EventSys::Put(preserved));EventSys::FlushEvents(Input::eventId);check(EventSys::Get(e)&&e.type==other);
  Input::Done();EventSys::Done();
  EventSys::Init();Input::Init(nullptr,nullptr);check(EventSys::QueueEmpty());Input::Done();EventSys::Done();
  std::cout<<"Native pointer/key events, pause releases, queue backpressure and selective flush passed\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
