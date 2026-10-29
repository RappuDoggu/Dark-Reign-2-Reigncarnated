#include "event_slice.hpp"
#include "reign/event_loop.hpp"
#include <iostream>
namespace native=reign::event_detail;
static Bool receive(native::Event &e){
 auto *sum=static_cast<U32*>(native::EventSys::GetContext(e.type));
 *sum+=e.iface.p1;return FALSE;
}
int main(){try{
 unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::logic_error("Native context check "+std::to_string(checks));};
 reign::EventLoop first,second;U16 a=0,b=0;U32 totalA=0,totalB=0;
 first.run_native([&]{native::EventSys::RegisterEvent("IFACE",a);native::EventSys::SetHandler(a,receive,&totalA);});
 second.run_native([&]{native::EventSys::RegisterEvent("IFACE",b);native::EventSys::SetHandler(b,receive,&totalB);});
 check(a!=0&&b!=0);
 first.run_native([&]{
  check(native::EventSys::GetContext(a)==&totalA);
  try{second.run_native([&]{check(native::EventSys::GetContext(b)==&totalB);throw std::runtime_error("intentional");});}catch(const std::runtime_error&){}
  check(native::EventSys::GetContext(a)==&totalA);
  auto *e=native::EventSys::PutLock();check(e!=nullptr);*e={};e->type=a;e->iface.p1=7;native::EventSys::PutUnlock();
 });
 first.process();check(totalA==7&&totalB==0&&first.empty());
 second.run_native([&]{native::Event e{};e.type=b;e.iface.p1=11;check(native::EventSys::Put(e));});
 second.process();check(totalA==7&&totalB==11);
 bool unbound=false;try{native::EventSys::QueueEmpty();}catch(const std::runtime_error&){unbound=true;}check(unbound);
 bool empty=false;try{first.run_native({});}catch(const std::invalid_argument&){empty=true;}check(empty);
 std::cout<<"Original EventSys scoped binding, raw queue, nested exception restoration: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
