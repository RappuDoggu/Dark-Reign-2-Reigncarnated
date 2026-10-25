#include "iface_events.hpp"
#include "reign/interface_events.hpp"
#include "reign/event_loop.hpp"
#include <array>
namespace reign::iface_event_detail {
namespace {thread_local State *active=nullptr;}
State &state(){if(!active)throw std::runtime_error("Original IFace event session not bound");return *active;}
Binding::Binding(State &s):previous(active){active=&s;}
Binding::~Binding(){active=previous;}
}
namespace reign {
namespace {
void dispose(void *to,void *from){delete static_cast<IControlPtr*>(to);delete static_cast<IControlPtr*>(from);}
}
struct InterfaceEvents::Impl {
 iface_event_detail::State state;
 std::unique_ptr<EventLoop> owned;
 EventLoop &events;
 explicit Impl(EventLoop *shared):owned(shared?nullptr:std::make_unique<EventLoop>()),events(shared?*shared:*owned){
  state.eventId=events.register_type("IFACE",EventPayload::interface);
  events.set_handler(state.eventId,[this](const EngineEvent &e){
   iface_event_detail::Binding binding(state);
   Event original{};original.type=e.type;original.subType=e.subtype;
   original.iface.to=e.to;original.iface.from=e.from;
   original.iface.p1=e.interface_parameters[0];original.iface.p2=e.interface_parameters[1];
   original.iface.p3=e.interface_parameters[2];original.iface.p4=e.interface_parameters[3];
   // Original handler deletes these on success. Retain exception-path ownership
   // because native error reporting can throw instead of terminating Windows.
   std::unique_ptr<IControlPtr> to(static_cast<IControlPtr*>(e.to)),from(static_cast<IControlPtr*>(e.from));
   const Bool result=IFace::IFaceEventHandler(original);
   to.release();from.release();return result!=FALSE;
  });
 }
 ~Impl(){
  // Remove only this interface's owned Reapers. Preserve other event types and
  // FIFO order in both the producer queue and an interrupted work batch.
  events.run_native([&]{
   std::array<Event,128> keep{};std::size_t count=0;Event e;
   while(event_detail::EventSys::Get(e)){
    if(e.type==state.eventId)dispose(e.iface.to,e.iface.from);
    else keep[count++]=e;
   }
   for(std::size_t i=0;i<count;++i){const Bool ok=event_detail::EventSys::Put(keep[i]);ASSERT(ok);}
   count=0;auto &work=event_detail::EventSys::state().workQueue;
   while(auto *next=work.RemovePre()){
    Event copy=*next;work.RemovePost();
    if(copy.type==state.eventId)dispose(copy.iface.to,copy.iface.from);
    else keep[count++]=copy;
   }
   for(std::size_t i=0;i<count;++i){auto *next=work.AddPre();ASSERT(next);*next=keep[i];work.AddPost();}
  });
  events.set_handler(state.eventId,{});
 }

};
InterfaceEvents::InterfaceEvents():impl_(std::make_unique<Impl>(nullptr)){}
InterfaceEvents::InterfaceEvents(EventLoop &events):impl_(std::make_unique<Impl>(&events)){}
InterfaceEvents::~InterfaceEvents()=default;
void InterfaceEvents::run(const std::function<void()> &call){
 if(!call)throw std::invalid_argument("Missing interface callback");
 iface_event_detail::Binding binding(impl_->state);impl_->events.run_native(call);
}
void InterfaceEvents::process(){run([&]{impl_->events.process();});}
bool InterfaceEvents::empty()const{
 bool result=false;
 impl_->events.run_native([&]{auto &s=event_detail::EventSys::state();result=s.queue.Empty()&&s.workQueue.Empty();});
 return result;
}
}
