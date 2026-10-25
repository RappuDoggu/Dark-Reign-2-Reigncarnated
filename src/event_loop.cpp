#include "event_slice.hpp"
#include "reign/event_loop.hpp"
#include <unordered_map>
namespace reign::event_detail::EventSys {
namespace {thread_local State *active=nullptr;}
State &state(){if(!active)throw std::runtime_error("Original event session not bound");return *active;}
struct Binding {State *previous;explicit Binding(State &s):previous(active){active=&s;}~Binding(){active=previous;}};
}
namespace reign {
namespace {
using Original=event_detail::Event;
struct Handler {EventPayload payload;std::function<bool(const EngineEvent&)> call;};
EngineEvent decode(const Original &e,EventPayload kind){EngineEvent n;n.type=e.type;n.subtype=e.subType;
 if(kind==EventPayload::input){n.code=e.input.code;n.character=e.input.ch;n.state=e.input.state;n.x=e.input.mouseX;n.y=e.input.mouseY;}
 else if(kind==EventPayload::interface){n.to=e.iface.to;n.from=e.iface.from;n.interface_parameters={e.iface.p1,e.iface.p2,e.iface.p3,e.iface.p4};}
 else n.parameters={e.param1,e.param2,e.param3,e.param4,e.param5,e.param6};return n;
}
Original encode(const EngineEvent &n,EventPayload kind){Original e{};e.type=n.type;e.subType=n.subtype;
 if(kind==EventPayload::input){e.input.code=n.code;e.input.ch=n.character;e.input.state=n.state;e.input.mouseX=n.x;e.input.mouseY=n.y;}
 else if(kind==EventPayload::interface){e.iface.to=n.to;e.iface.from=n.from;e.iface.p1=n.interface_parameters[0];e.iface.p2=n.interface_parameters[1];e.iface.p3=n.interface_parameters[2];e.iface.p4=n.interface_parameters[3];}
 else{e.param1=n.parameters[0];e.param2=n.parameters[1];e.param3=n.parameters[2];e.param4=n.parameters[3];e.param5=n.parameters[4];e.param6=n.parameters[5];}return e;
}
Bool dispatch(Original &e){auto *handler=static_cast<Handler *>(event_detail::EventSys::GetContext(e.type));return handler&&handler->call?handler->call(decode(e,handler->payload)):FALSE;}
}
struct EventLoop::Impl {
 event_detail::EventSys::State state;std::unordered_map<std::uint16_t,std::unique_ptr<Handler>> handlers;bool processing=false;
 Impl(){event_detail::EventSys::Binding b(state);event_detail::EventSys::Init();}
 ~Impl(){event_detail::EventSys::Binding b(state);event_detail::EventSys::Done();}
 EventPayload payload(std::uint16_t id)const{const auto i=handlers.find(id);return i==handlers.end()?EventPayload::generic:i->second->payload;}
};
EventLoop::EventLoop():impl_(std::make_unique<Impl>()){}
EventLoop::~EventLoop()=default;
std::uint16_t EventLoop::register_type(const std::string &name,EventPayload payload){
 if(name.empty()||name.size()>15||name.find('\0')!=std::string::npos)throw std::runtime_error("Event name must contain 1..15 bytes without NUL");
 auto &i=*impl_;if(i.state.nextEventId==0)throw std::runtime_error("Event ID space exhausted");event_detail::EventSys::Binding b(i.state);U16 id;auto handler=std::make_unique<Handler>();handler->payload=payload;event_detail::EventSys::RegisterEvent(name.c_str(),id);i.handlers.emplace(id,std::move(handler));return id;
}
void EventLoop::set_handler(std::uint16_t id,std::function<bool(const EngineEvent&)> callback){auto &i=*impl_;auto found=i.handlers.find(id);if(found==i.handlers.end())throw std::runtime_error("Unknown event type");event_detail::EventSys::Binding b(i.state);found->second->call=std::move(callback);event_detail::EventSys::SetHandler(id,dispatch,found->second.get());}
bool EventLoop::put(const EngineEvent &event){auto &i=*impl_;auto original=encode(event,i.payload(event.type));event_detail::EventSys::Binding b(i.state);return event_detail::EventSys::Put(original);}
bool EventLoop::get(EngineEvent &event){auto &i=*impl_;Original original;event_detail::EventSys::Binding b(i.state);if(!event_detail::EventSys::Get(original))return false;event=decode(original,i.payload(original.type));return true;}
bool EventLoop::empty()const{event_detail::EventSys::Binding b(impl_->state);return event_detail::EventSys::QueueEmpty();}
bool EventLoop::full()const{event_detail::EventSys::Binding b(impl_->state);return event_detail::EventSys::QueueFull();}
void EventLoop::process(){auto &i=*impl_;if(i.processing)throw std::runtime_error("Event dispatch cannot reenter");event_detail::EventSys::Binding b(i.state);i.processing=true;try{event_detail::EventSys::ProcessAll();}catch(...){i.processing=false;throw;}i.processing=false;}
void EventLoop::flush(std::uint16_t type){event_detail::EventSys::Binding b(impl_->state);event_detail::EventSys::FlushEvents(type);}
}
