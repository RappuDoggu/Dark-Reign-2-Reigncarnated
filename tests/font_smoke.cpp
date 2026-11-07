#include "reign/native_font.hpp"
#include "font_fixture.hpp"
#include <cmath>
#include <cfenv>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool b,const char *s){if(!b)throw std::runtime_error(s);}
template<class F>void rejects(F f){try{f();}catch(const std::runtime_error&){return;}throw std::runtime_error("Invalid font operation accepted");}
}
int main(){try{
 for(unsigned version:{1u,2u}){
  auto file=font_fixture(version);reign::NativeFont font(file);
  check(font.version()==version&&font.glyph_count()==2&&font.height()==4&&font.width(u"AB?")==6&&font.atlas_count()==1,"Original font metadata");
  const auto &atlas=font.atlas(0);auto pixel=[&](unsigned x,unsigned y,unsigned channel){return atlas.pixels[(y*atlas.width+x)*4+channel];};
  check(atlas.width==128&&atlas.height==128&&pixel(1,1,0)==255&&pixel(1,1,3)==255&&pixel(2,1,3)==128&&pixel(1,2,3)==64&&pixel(2,2,3)==0&&pixel(0,0,3)==0,"Original glyph atlas packing/alpha/gutter");
  std::vector<reign::FontBatch> batches;reign::FontDrawOptions options;options.x=10;options.y=20;
  auto collect=[&](const auto &batch){batches.push_back(batch);};font.draw(u"AB",options,collect);
  check(batches.size()==1&&batches[0].vertices.size()==8&&batches[0].indices==std::vector<std::uint16_t>({0,1,2,0,2,3,4,5,6,4,6,7}),"Original IFace batching/index offset");
  const auto &v=batches[0].vertices;check(v[0].x==10&&v[0].y==20&&v[4].x==13&&v[0].u==1.5f/128&&!batches[0].filtered,"Original glyph geometry/UV");
  batches.clear();options.batch_vertices=8;font.draw(u"AB",options,collect);check(batches.size()==2,"Original IFace capacity flush");
  batches.clear();options={};options.alpha_scale=.5f;options.shadow=1;options.shadow_alpha=77;int rounding=std::fegetround();std::fesetround(FE_UPWARD);font.draw(u"A",options,collect);check(std::fegetround()==FE_UPWARD,"Font draw changed caller rounding mode");std::fesetround(rounding);
  check(batches.size()==1&&batches[0].vertices.size()==8&&(batches[0].vertices[0].diffuse>>24)==127&&batches[0].vertices[4].diffuse==0x4d000000&&batches[0].vertices[4].x==1,"Original alpha/shadow rules");
  batches.clear();options={};options.clip={{0,0},{1,128}};font.draw(u"AB",options,collect);check(batches.empty(),"Original right clip early-return");
  unsigned uploads=0;font.upload([&](unsigned id,const reign::RgbaImage &image){check(id==0&&image.pixels==atlas.pixels,"Atlas replay bytes");++uploads;});font.upload([&](unsigned,const auto&){++uploads;});check(uploads==2,"Atlas replay after context recreation");
  rejects([&]{font.draw(u"AB",{},[](const auto&){throw std::runtime_error("Consumer failure");});});batches.clear();font.draw(u"A",{},collect);check(batches.size()==1,"Draw recovery after consumer failure");
  rejects([&]{font.draw(u"A",{},[&](const auto&){font.draw(u"A",{},collect);});});
  rejects([&]{font.upload({});});
  for(std::size_t n=0;n<file.size();++n)rejects([&]{reign::NativeFont truncated({file.begin(),file.begin()+n});});
  auto bad=file;bad[0]=0;rejects([&]{reign::NativeFont malformed(bad);});
 }
 // Four large glyphs require distinct original 128x128 atlas pages.
 reign::NativeFont paged(font_fixture(2,4,100));check(paged.atlas_count()==4,"Original multi-atlas rollover");
 std::vector<reign::FontBatch> batches;paged.draw(u"ABCD",{},[&](const auto &b){batches.push_back(b);});check(batches.size()==4,"Original IFace texture-change flush");
 std::cout<<"Original FFF v1/v2 validation, glyph atlas/metrics/draw/shadow/clipping/capacity/texture flush/replay/recovery PASS\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
