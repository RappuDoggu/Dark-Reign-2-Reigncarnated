#include "reign/frontend_draw.hpp"
#include <iostream>
int main(){try{
 unsigned checks=0;auto check=[&](bool ok){++checks;if(!ok)throw std::logic_error("UI drawing check "+std::to_string(checks));};
 auto rejects=[&](auto call){bool caught=false;try{call();}catch(const std::runtime_error&){caught=true;}check(caught);};
 Point<S32> points[3]={{2,3},{12,3},{7,9}};
 auto draw=[&]{IFace::RenderTriangle(points,Color(U32(0x80402010)),.5f,nullptr);};
 rejects(draw);unsigned batches=0;
 auto sink=[&](const auto &v,const auto &i){++batches;check(v.size()==3&&i==std::vector<std::uint16_t>({0,1,2}));
  for(unsigned n=0;n<3;++n){check(v[n].x==points[n].x&&v[n].y==points[n].y);check(v[n].z==0&&v[n].rhw==1);check(v[n].diffuse==0x40402010&&v[n].specular==0xff000000);check(v[n].u==0&&v[n].v==0);}
 };
 reign::FrontendDraw::run([&]{draw();draw();},sink);check(batches==2);
 reign::FrontendDraw::run([&]{IFace::RenderTriangle(points,Color(U32(0)),1,nullptr);},sink);check(batches==2);
 rejects([&]{reign::FrontendDraw::run([&]{draw();throw std::runtime_error("collection failed");},sink);});check(batches==2);
 rejects([&]{reign::FrontendDraw::run(draw,[](const auto &,const auto &){throw std::runtime_error("submission failed");});});
 reign::FrontendDraw::run([&]{rejects([&]{reign::FrontendDraw::run([]{throw std::runtime_error("nested");},sink);});draw();},sink);check(batches==3);
 rejects(draw);
 std::cout<<"Original UI triangle generation/native submission/recovery: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
