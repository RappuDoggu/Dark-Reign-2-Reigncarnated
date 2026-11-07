#include "input_native.h"
#include "event.h"
#include <deque>
#include <mutex>
#include <array>
namespace {
struct Action { U16 type;S32 code,x,y;CH character; };
std::mutex mutex;
std::deque<Action> pending;
std::array<bool,256> keys{};
bool initialized=false,active=false,visible=true;
S32 width=640,height=480;
Point<S32> ratio(65536,65536);
void Append(Action action){if(pending.size()>=4096)throw std::overflow_error("Native input backlog");pending.push_back(action);}
U32 ButtonMask(S32 code){switch(code){case 0:return Input::LBUTTONDOWN;case 1:return Input::RBUTTONDOWN;case 2:return Input::MBUTTONDOWN;}throw std::invalid_argument("Mouse button code");}
U32 KeyMask(S32 code){switch(code){case 0x2a:return Input::LSHIFTDOWN;case 0x36:return Input::RSHIFTDOWN;case 0x1d:return Input::LCTRLDOWN;case 0x9d:return Input::RCTRLDOWN;case 0x38:return Input::LALTDOWN;case 0xb8:return Input::RALTDOWN;case 0xdb:return Input::LWINDOWN;case 0xdc:return Input::RWINDOWN;default:return 0;}}
}
namespace Input {
U16 eventId=0;
Point<S32> mousePos(0,0),mouseDelta(0,0);
U32 customState=0,lButton=0,rButton=1,mButton=2;
void Init(HINSTANCE,HWND){std::lock_guard<std::mutex> lock(mutex);if(initialized)throw std::logic_error("Input already initialized");EventSys::RegisterEvent("INPUT",eventId);keys.fill(false);customState=0;pending.clear();initialized=active=true;}
void FlushEvents(){std::lock_guard<std::mutex> lock(mutex);pending.clear();if(initialized)EventSys::FlushEvents(eventId);}
void Done(){FlushEvents();std::lock_guard<std::mutex> lock(mutex);keys.fill(false);customState=0;initialized=active=false;eventId=0;}
void SetMousePos(S32 x,S32 y){std::lock_guard<std::mutex> lock(mutex);mousePos.x=std::clamp(x,S32(0),width-1);mousePos.y=std::clamp(y,S32(0),height-1);}
void SetMouseAxis(S32 delta){NativeEngine::QueueWheel(delta);}
void ShowCursor(Bool show){visible=show!=FALSE;}
Bool CursorVisible(){return visible?TRUE:FALSE;}
Bool IsKeyDown(int code){std::lock_guard<std::mutex> lock(mutex);return code>=0&&code<256&&keys[code]?TRUE:FALSE;}
Bool KeyState(KeyStateCode code){std::lock_guard<std::mutex> lock(mutex);return (customState&code)?TRUE:FALSE;}
const Point<S32> &GetModeRatio(){return ratio;}
void OnModeChange(){std::lock_guard<std::mutex> lock(mutex);ratio.x=S32((std::int64_t(width)<<16)/640);ratio.y=S32((std::int64_t(height)<<16)/480);mousePos.x=std::clamp(mousePos.x,S32(0),width-1);mousePos.y=std::clamp(mousePos.y,S32(0),height-1);}
void OnActivate(Bool enabled){
 std::lock_guard<std::mutex> lock(mutex);if(!initialized)return;
 if(!enabled){
  pending.clear();EventSys::FlushEvents(eventId);
  for(S32 button=0;button<3;++button)if(customState&ButtonMask(button))Append({MOUSEBUTTONUP,button,mousePos.x,mousePos.y,0});
  for(S32 key=0;key<256;++key)if(keys[key])Append({KEYUP,key,0,0,0});
  customState=0;keys.fill(false);
 }
 active=enabled!=FALSE;
}
void ReadEvents(){
 std::lock_guard<std::mutex> lock(mutex);if(!initialized)throw std::logic_error("Input not initialized");mouseDelta.Set(0,0);
 while(!pending.empty()){
  const Action a=pending.front();Event event{};event.type=eventId;event.subType=a.type;
  U32 nextState=customState;auto nextPos=mousePos;
  if(a.type==MOUSEMOVE||a.type==MOUSEBUTTONDOWN||a.type==MOUSEBUTTONUP){nextPos.x=std::clamp(a.x,S32(0),width-1);nextPos.y=std::clamp(a.y,S32(0),height-1);}
  if(a.type==MOUSEBUTTONDOWN)nextState|=ButtonMask(a.code);
  if(a.type==MOUSEBUTTONUP)nextState&=~ButtonMask(a.code);
  if(a.type==KEYDOWN){if(keys[a.code])event.subType=KEYREPEAT;nextState|=KeyMask(a.code);}
  if(a.type==KEYUP)nextState&=~KeyMask(a.code);
  event.param1=(U32(a.character)<<16)|U16(a.code);event.param2=nextState;event.param3=U32(nextPos.x);event.param4=U32(nextPos.y);
  if(!EventSys::Put(event))break; // Keep the action until queue space is available.
  mouseDelta.x+=S32(std::int64_t(nextPos.x-mousePos.x)*640/width);
  mouseDelta.y+=S32(std::int64_t(nextPos.y-mousePos.y)*480/height);
  mousePos=nextPos;customState=nextState;
  if(a.type==KEYDOWN)keys[a.code]=true;if(a.type==KEYUP)keys[a.code]=false;
  pending.pop_front();
 }
}
}
namespace NativeEngine {
void SetInputViewport(S32 w,S32 h){if(w<=0||h<=0||w>32767||h>32767)throw std::invalid_argument("Input viewport dimensions");{std::lock_guard<std::mutex> lock(mutex);width=w;height=h;}Input::OnModeChange();}
void QueuePointer(S32 x,S32 y,S32 button,Bool pressed){std::lock_guard<std::mutex> lock(mutex);if(!initialized||!active)return;if(button < -1||button>2)throw std::invalid_argument("Pointer button");Append({U16(button<0?Input::MOUSEMOVE:pressed?Input::MOUSEBUTTONDOWN:Input::MOUSEBUTTONUP),button<0?0:button,x,y,0});}
void QueueKey(U8 code,Bool pressed,CH ch){std::lock_guard<std::mutex> lock(mutex);if(!initialized||!active)return;if(pending.size()+(pressed&&ch?2:1)>4096)throw std::overflow_error("Native input backlog");Append({U16(pressed?Input::KEYDOWN:Input::KEYUP),code,0,0,0});if(pressed&&ch)Append({Input::KEYCHAR,code,0,0,ch});}
void QueueWheel(S32 delta){if(delta<std::numeric_limits<S16>::min()||delta>std::numeric_limits<S16>::max())throw std::out_of_range("Wheel delta");std::lock_guard<std::mutex> lock(mutex);if(initialized&&active)Append({Input::MOUSEAXIS,0,0,0,CH(S16(delta))});}
}
