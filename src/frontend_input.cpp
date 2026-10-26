#include "reign/frontend_input.hpp"
#include "reign/frontend_prelude.hpp"
#include "control_input.hpp"
namespace Input {
thread_local U16 eventId=0;
thread_local Point<S32> mousePos{};
thread_local U32 lButton=0;
thread_local U32 rButton=1;
}
namespace reign {
struct FrontendInput::Impl {
 static thread_local Impl *active;
 EventLoop &events;U16 id;S32 x=0,y=0;U32 buttons=0;bool bound=false;
 explicit Impl(EventLoop &e):events(e),id(e.register_type("INPUT",EventPayload::input)){}
 bool post(U16 kind,unsigned code,S32 nx,S32 ny,U32 state){
  if(!bound||active!=this)throw std::runtime_error("Frontend input not bound");
  EngineEvent event;event.type=id;event.subtype=kind;event.code=static_cast<std::int16_t>(code);event.x=nx;event.y=ny;event.state=state;
  if(!events.put(event))return false;
  x=nx;y=ny;buttons=state;Input::mousePos.Set(x,y);return true;
 }
};
thread_local FrontendInput::Impl *FrontendInput::Impl::active=nullptr;
FrontendInput::FrontendInput(EventLoop &events):impl_(std::make_unique<Impl>(events)){}
FrontendInput::~FrontendInput()=default;
std::uint16_t FrontendInput::event_id()const{return impl_->id;}
void FrontendInput::run(const std::function<void()> &call){
 if(!call)throw std::invalid_argument("Missing input callback");
 // Disallow nested binding of this producer: restoring its old coordinates
 // after accepted events would otherwise hide its newer pointer state.
 if(impl_->bound)throw std::logic_error("Input producer already bound");
 auto *oldActive=Impl::active;Impl::active=impl_.get();
 const auto oldId=Input::eventId;const auto oldPos=Input::mousePos;const auto oldButton=Input::lButton;
 Input::eventId=impl_->id;Input::mousePos.Set(impl_->x,impl_->y);Input::lButton=0;impl_->bound=true;
 struct Restore{Impl &self;Impl *active;U16 id;Point<S32> pos;U32 button;~Restore(){Impl::active=active;self.bound=false;Input::eventId=id;Input::mousePos=pos;Input::lButton=button;}} restore{*impl_,oldActive,oldId,oldPos,oldButton};
 impl_->events.run_native(call);
}
bool FrontendInput::character(unsigned ascii){
 if(!impl_->bound||Impl::active!=impl_.get())throw std::runtime_error("Frontend input not bound");
 if(ascii<32||ascii>126)throw std::invalid_argument("Initial menu text input supports printable ASCII only");
 EngineEvent e;e.type=impl_->id;e.subtype=Input::KEYCHAR;e.character=static_cast<std::int16_t>(ascii);return impl_->events.put(e);
}
bool FrontendInput::key(unsigned scan,bool down){
 if(!impl_->bound||Impl::active!=impl_.get())throw std::runtime_error("Frontend input not bound");
 if(scan>255)throw std::invalid_argument("Scan code outside original range");
 EngineEvent e;e.type=impl_->id;e.subtype=down?Input::KEYDOWN:Input::KEYUP;e.code=static_cast<std::int16_t>(scan);return impl_->events.put(e);
}
bool FrontendInput::move(std::int32_t x,std::int32_t y){return impl_->post(Input::MOUSEMOVE,0,x,y,impl_->buttons);}
bool FrontendInput::button(unsigned button,bool down,std::int32_t x,std::int32_t y){
 if(button>2)throw std::invalid_argument("Mouse button outside supported range");
 const U32 masks[]={Input::LBUTTONDOWN,Input::RBUTTONDOWN,Input::MBUTTONDOWN};
 U32 state=down?(impl_->buttons|masks[button]):(impl_->buttons&~masks[button]);
 return impl_->post(down?Input::MOUSEBUTTONDOWN:Input::MOUSEBUTTONUP,button,x,y,state);
}
}
