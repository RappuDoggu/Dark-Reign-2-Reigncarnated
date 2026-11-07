#include "reign/frontend_input.hpp"
#include "reign/frontend_prelude.hpp"
#include "control_input.hpp"
#include <iostream>
int main(){try{
 unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("Input check "+std::to_string(checks));};
 auto rejects=[&](auto f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);};
 reign::EventLoop events,otherEvents;reign::FrontendInput input(events),other(otherEvents);std::vector<reign::EngineEvent> seen;
 events.set_handler(input.event_id(),[&](const auto &e){seen.push_back(e);return false;});
 rejects([&]{input.move(1,2);});check(Input::eventId==0);
 input.run([&]{check(Input::eventId==input.event_id()&&Input::lButton==0);check(input.move(-2,100));check(Input::mousePos.x==-2);check(input.button(0,true,3,4));check(input.move(5,6));check(input.button(0,false,7,8));events.process();
  check(seen.size()==4&&seen[0].subtype==Input::MOUSEMOVE&&seen[0].x==-2&&seen[0].y==100);
  check(seen[1].subtype==Input::MOUSEBUTTONDOWN&&seen[1].code==0&&seen[1].state==Input::LBUTTONDOWN);
  check(seen[2].state==Input::LBUTTONDOWN&&seen[3].state==0&&seen[3].subtype==Input::MOUSEBUTTONUP);
  other.run([&]{check(Input::mousePos.x==0);other.move(90,91);rejects([&]{input.move(1,1);});});check(Input::mousePos.x==7&&Input::mousePos.y==8);
  rejects([&]{input.run([]{});});rejects([&]{input.button(3,true,0,0);});
  for(unsigned i=0;i<128;++i)check(input.move(i,0));check(!input.button(0,true,999,0));check(Input::mousePos.x==127);
  events.process();check(input.button(0,true,9,10));check(input.button(0,false,9,10));events.process();
 });check(Input::eventId==0&&Input::mousePos.x==0);
 rejects([&]{input.run([]{throw std::runtime_error("intentional");});});check(Input::eventId==0);
 input.run([&]{check(Input::mousePos.x==9&&Input::mousePos.y==10);});
 std::cout<<"Native menu input/original queue payload/state/backpressure: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
