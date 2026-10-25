#include "reign/frontend_frame.hpp"
#include "reign/frontend_prelude.hpp"
#include <iostream>
#include <limits>
int main(){try{
 unsigned checks=0;auto check=[&](bool ok){++checks;if(!ok)throw std::logic_error("Frame check "+std::to_string(checks));};
 auto rejects=[&](auto f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}check(caught);};
 reign::InterfaceMetrics metrics(640,480),otherMetrics(320,240);
 reign::FrontendFrame frame(metrics),other(otherMetrics);
 rejects([]{IFace::ScreenWidth();});rejects([]{IFace::GetMetric(IFace::BUTTON_WIDTH);});
 frame.run([&]{
  check(IFace::ScreenWidth()==640&&IFace::ScreenHeight()==480&&IFace::ScreenDepth()==32);
  check(IFace::GetMetric(IFace::BUTTON_WIDTH)==metrics.get("ButtonWidth"));
  check(IFace::GetMetric(IFace::SHADOW_ALPHA)==metrics.get("ShadowAlpha"));
  for(U32 i=0;i<IFace::MAX_METRICS;++i)check(IFace::GetMetric(i)==metrics.get_id(i));
  rejects([]{IFace::GetMetric(IFace::MAX_METRICS);});rejects([]{IFace::GetMetric(0xffffffffu);});
  rejects([]{IFace::TimeStepMs();});rejects([]{IFace::TimeStepSec();});rejects([]{IFace::ElapsedMs();});
 });
 frame.set_time({100,16,.016f});other.set_time({999,33,.033f});
 frame.run([&]{
  check(IFace::ElapsedMs()==100&&IFace::TimeStepMs()==16&&IFace::TimeStepSec()==.016f);
  rejects([&]{other.run([&]{check(IFace::ScreenWidth()==320&&IFace::TimeStepMs()==33);throw std::runtime_error("nested");});});
  check(IFace::ScreenWidth()==640&&IFace::TimeStepMs()==16);
  metrics.resize(1692,666);check(IFace::ScreenWidth()==1692&&IFace::ScreenHeight()==666);
  check(IFace::GetMetric(IFace::SCREEN_WIDTH)==1692&&IFace::GetMetric(IFace::SCREEN_HEIGHT)==666);
 });
 frame.set_time({0xffffffffu,0,0});frame.run([&]{check(IFace::ElapsedMs()==0xffffffffu&&IFace::TimeStepMs()==0);});
 frame.set_time({0,17,.017f});frame.run([&]{check(IFace::ElapsedMs()==0&&IFace::TimeStepMs()==17);});
 for(float invalid:{-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
  bool caught=false;try{frame.set_time({1,2,invalid});}catch(const std::invalid_argument&){caught=true;}check(caught);
 }
 frame.run([&]{check(IFace::TimeStepSec()==.017f&&IFace::TimeStepMs()==17);});
 rejects([]{IFace::TimeStepMs();});
 std::cout<<"Frontend original metrics/viewport/engine timing binding: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
