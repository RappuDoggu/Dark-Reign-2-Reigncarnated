#include "reign/frontend_font.hpp"
#include "font_fixture.hpp"
#include "reign/frontend_draw.hpp"
#include <iostream>
int main(){try{
 unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::logic_error("Font bridge check "+std::to_string(checks));};
 auto rejects=[&](auto f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}check(caught);};
 reign::FontRegistry registry([](const std::string &){return font_fixture();});
 registry.create("System","a.fff");registry.create("Alias","a.fff");
 std::vector<int> order;
 std::vector<reign::FontBatch> batches;const reign::NativeFont *submitted=nullptr;bool fail=false;
 reign::FrontendFonts bridge(registry,[&](const auto &font,const auto &batch){if(fail)throw std::runtime_error("Injected consumer failure");submitted=&font;batches.push_back(batch);order.push_back(2);});
 const CH letters[]={65,66,65};Font *old=nullptr;
 rejects([]{FontSys::GetFont("System");});
 bridge.run([&]{
  old=FontSys::GetFont("System");check(old&&old==FontSys::GetFont("ALIAS"));
  check(old==FontSys::GetFont(Crc::CalcStr("system")));check(!FontSys::GetFont("Absent"));
  check(old->Height()==4&&old->Width(letters,2)==6);check(old->Width(nullptr,0)==0&&old->Width(nullptr,-1)==0);
  rejects([&]{old->Width(nullptr,1);});rejects([&]{old->Draw(0,0,letters,2,Color(U32(0xffffffff)));});
 });
 bridge.set_frame({{0,0},{64,64}},128);
 bridge.run([&]{
  old->Draw(4,5,letters,2,Color(U32(0xff123456)),nullptr,.5f,1);
  check(!batches.empty()&&submitted==&old->native());
  std::vector<reign::FontBatch> expected;reign::FontDrawOptions options;options.x=4;options.y=5;options.argb=0xff123456;
  options.alpha_scale=.5f;options.shadow=1;options.clip={{0,0},{64,64}};options.shadow_alpha=128;
  old->native().draw(u"AB",options,[&](const auto &b){expected.push_back(b);});
  check(expected.size()==batches.size());
  for(std::size_t i=0;i<expected.size();++i){check(expected[i].indices==batches[i].indices);check(expected[i].vertices.size()==batches[i].vertices.size());
   check(expected[i].atlas==batches[i].atlas&&expected[i].filtered==batches[i].filtered&&expected[i].flags==batches[i].flags);
   for(std::size_t j=0;j<expected[i].vertices.size();++j){const auto &a=expected[i].vertices[j],&b=batches[i].vertices[j];check(a.x==b.x&&a.y==b.y&&a.z==b.z&&a.rhw==b.rhw&&a.diffuse==b.diffuse&&a.specular==b.specular&&a.u==b.u&&a.v==b.v);}
  }
  ClipRect outside;outside.p0={30,30};outside.p1={40,40};batches.clear();
  old->Draw(0,0,letters,2,Color(U32(0xffffffff)),&outside);check(batches.empty());
  reign::FontRegistry other([](const std::string &){return font_fixture(1,1);});other.create("System","b.fff");
  reign::FrontendFonts nested(other,[](const auto &,const auto &){});
  rejects([&]{nested.run([&]{check(FontSys::GetFont("System")!=old);throw std::runtime_error("nested unwind");});});
  check(FontSys::GetFont("System")==old);
  fail=true;rejects([&]{old->Draw(0,0,letters,1,Color(U32(0xffffffff)));});fail=false;
  batches.clear();old->Draw(0,0,letters,1,Color(U32(0xffffffff)));check(!batches.empty());
  registry.reload();auto *fresh=FontSys::GetFont("System");check(fresh&&fresh!=old);check(old->Width(letters,2)==6);
  registry.clear();check(!FontSys::GetFont("System"));check(old->Height()==4);
 });
 rejects([]{FontSys::GetFont("System");});
 order.clear();bridge.run([&]{
 reign::FrontendDraw::run_batches([&]{
  IFace::RenderRectangle(ClipRect(0,0,8,8),Color(U32(0xffff0000)),nullptr,1);
  old->Draw(1,1,letters,2,Color(U32(0xffffffff)));
  IFace::RenderRectangle(ClipRect(8,0,16,8),Color(U32(0xff00ff00)),nullptr,1);
 },[&](const auto &batch){order.push_back(batch.vertices[0].diffuse==0xffff0000?1:3);});
 });check(order==std::vector<int>({1,2,3}));
 std::cout<<"Frontend Font adapter/original draw equivalence/lifetime: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
