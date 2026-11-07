#include "reign/event_loop.hpp"
#include "event_slice.hpp"
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>
int main(){try{
 unsigned checks=0;auto check=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Event check "+std::to_string(checks));};
 using E=reign::event_detail::Event;static_assert(sizeof(E)==40&&offsetof(E,iface)==8);static_assert(alignof(E)>=alignof(void*));E aligned{};check(reinterpret_cast<std::uintptr_t>(&aligned.iface.to)%alignof(void*)==0);
 reign::EventLoop loop;auto generic=loop.register_type("GENERIC"),input=loop.register_type("INPUT",reign::EventPayload::input),iface=loop.register_type("IFACE",reign::EventPayload::interface);
 check(generic==1&&input==2&&iface==3&&loop.empty()&&!loop.full());
 unsigned handled=0;std::vector<unsigned> sequence;
 loop.set_handler(generic,[&](const auto &e){++handled;sequence.push_back(e.parameters[0]);return true;});
 reign::EngineEvent e;e.type=generic;
 for(unsigned i=0;i<128;++i){e.parameters[0]=i;check(loop.put(e));}check(loop.full()&&!loop.empty()&&!loop.put(e));loop.process();check(handled==127&&!loop.empty());loop.process();check(handled==128&&loop.empty());for(unsigned i=0;i<128;++i)check(sequence[i]==i);
 e.type=input;e.code=-7;e.character=123;e.state=0xf0000080;e.x=-128;e.y=1692;check(loop.put(e));reign::EngineEvent out;check(loop.get(out)&&out.code==-7&&out.character==123&&out.state==e.state&&out.x==-128&&out.y==1692);
 int destination=0,origin=1;e={};e.type=iface;e.to=&destination;e.from=&origin;e.interface_parameters={1,2,0xffffffffu,4};check(loop.put(e));check(loop.get(out)&&out.to==&destination&&out.from==&origin&&out.interface_parameters==e.interface_parameters);
 e.type=generic;loop.put(e);e.type=input;loop.put(e);loop.flush(generic);check(loop.empty());
 unsigned deferred=0;loop.set_handler(generic,[&](const auto &event){++deferred;if(deferred==1)loop.put(event);return true;});e={};e.type=generic;loop.put(e);loop.process();check(deferred==1&&!loop.empty());loop.process();check(deferred==2&&loop.empty());
 loop.set_handler(generic,[&](const auto &){throw std::runtime_error("injected handler failure");return true;});loop.put(e);bool caught=false;try{loop.process();}catch(const std::exception &){caught=true;}check(caught);loop.set_handler(generic,{});loop.process();check(loop.empty());
 reign::EventLoop concurrent;auto id=concurrent.register_type("PRODUCER");std::atomic<unsigned> done{0};std::vector<std::thread> producers;
 for(unsigned p=0;p<4;++p)producers.emplace_back([&,p]{for(unsigned i=0;i<200;++i){reign::EngineEvent n;n.type=id;n.parameters={p,i,0,0,0,0};while(!concurrent.put(n))std::this_thread::yield();}++done;});
 bool seen[4][200]{};unsigned received=0;while(received<800){reign::EngineEvent n;if(concurrent.get(n)){check(n.parameters[0]<4&&n.parameters[1]<200&&!seen[n.parameters[0]][n.parameters[1]]);seen[n.parameters[0]][n.parameters[1]]=true;++received;}else std::this_thread::yield();}
 for(auto &p:producers)p.join();check(done==4&&concurrent.empty());
 check(sizeof(E)==40&&offsetof(E,iface)==8);std::cout<<"Original EventSys dispatch/pointer payload/FIFO/deferred/128 capacity/800 producer events: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
