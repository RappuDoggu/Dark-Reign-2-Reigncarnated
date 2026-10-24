#include "reign/renderer_checks.hpp"
#include <array>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <vector>
namespace reign {
namespace {
constexpr unsigned W=32,H=32;
std::array<LegacyTL,4> quad(std::uint32_t color,float z=.5f,std::uint32_t spec=0xff000000u){return {{{-.5f,-.5f,z,1,color,spec,0,0},{W-.5f,-.5f,z,1,color,spec,1,0},{W-.5f,H-.5f,z,1,color,spec,1,1},{-.5f,H-.5f,z,1,color,spec,0,1}}};}
const std::uint16_t indices[]={0,1,2,0,2,3};
}
std::string renderer_checks(LegacyGles &renderer){
 int failed=0;std::ostringstream report;
  auto check=[&](bool b,const char *name){report<<name<<": "<<(b?"PASS":"FAIL")<<'\n';if(!b)++failed;};
  std::vector<std::uint8_t> pixels(W*H*4);
  auto pixel=[&](unsigned x,unsigned y,int r,int g,int b){renderer.read_rgba(W,H,pixels.data());auto p=((H-1-y)*W+x)*4;return std::abs(int(pixels[p])-r)<=1&&std::abs(int(pixels[p+1])-g)<=1&&std::abs(int(pixels[p+2])-b)<=1;};
  auto draw=[&](auto &v,const auto &s){renderer.draw(W,H,v.data(),v.size(),indices,6,s);};
  reign::DrawState state;auto red=quad(0xffff0000);renderer.clear(W,H,0,0,0);draw(red,state);
  check(pixel(0,0,255,0,0)&&pixel(31,31,255,0,0),"ARGB byte order and half-pixel coverage");
  const std::uint8_t texels[]={255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,255};auto texture=renderer.texture_rgba(2,2,texels);
  auto white=quad(0xffffffff);state.texture=texture;state.texture_op=reign::TextureOp::modulate_rgba;renderer.clear(W,H,0,0,0);draw(white,state);
  check(pixel(8,8,255,0,0)&&pixel(24,8,0,255,0)&&pixel(8,24,0,0,255),"D3D top-left texture orientation");
  state={};state.blend=reign::Blend::alpha;auto alpha=quad(0x80ff0000);renderer.clear(W,H,0,0,1);draw(alpha,state);check(pixel(16,16,128,0,127),"Source-alpha blending");
  state.alpha_test=true;state.alpha_reference=.75f;renderer.clear(W,H,0,0,1);draw(alpha,state);check(pixel(16,16,0,0,255),"Alpha reference discard");
  state={};state.depth_test=true;state.depth_write=true;auto near=quad(0xffff0000,.2f),far=quad(0xff0000ff,.8f);renderer.clear(W,H,0,0,0);draw(near,state);draw(far,state);check(pixel(16,16,255,0,0),"Zero-to-one depth mapping and occlusion");
  state={};state.fog=true;state.fog_rgb[1]=1;auto fog=quad(0xffff0000,.5f,0);renderer.clear(W,H,0,0,0);draw(fog,state);check(pixel(16,16,0,255,0),"Specular-alpha vertex fog");
  state={};state.specular=true;auto spec=quad(0xff800000,.5f,0xff000040);renderer.clear(W,H,0,0,0);draw(spec,state);check(pixel(16,16,128,0,64),"Specular RGB addition");
  state={};auto rhs=quad(0xffff0000);rhs[1].rhw=.25f;rhs[2].rhw=.5f;renderer.clear(W,H,0,0,0);draw(rhs,state);check(pixel(16,16,255,0,0),"Reciprocal homogeneous W path");
  bool rejected=false;const std::uint16_t bad[]={0,1,99};try{renderer.draw(W,H,red.data(),red.size(),bad,3,state);}catch(const std::exception &){rejected=true;}check(rejected,"Invalid index rejection");
  std::array<std::uint8_t,4*4*4> glyph{};for(unsigned n=0;n<16;++n)glyph[n*4]=glyph[n*4+1]=glyph[n*4+2]=255;
  glyph[(1*4+1)*4+3]=255;glyph[(1*4+2)*4+3]=128;glyph[(2*4+1)*4+3]=64;
  auto glyph_texture=renderer.texture_rgba(4,4,glyph.data());auto integer_quad=quad(0xffffffff);
  integer_quad[0].x=integer_quad[3].x=4;integer_quad[1].x=integer_quad[2].x=6;
  integer_quad[0].y=integer_quad[1].y=4;integer_quad[2].y=integer_quad[3].y=6;
  for(auto &v:integer_quad){v.u=.375f+(v.x-4)*.25f;v.v=.375f+(v.y-4)*.25f;}
  state={};state.texture=glyph_texture;state.texture_op=TextureOp::modulate_rgba;state.blend=Blend::alpha;renderer.clear(W,H,0,0,0);draw(integer_quad,state);
  check(pixel(4,4,255,255,255)&&pixel(5,4,128,128,128)&&pixel(4,5,64,64,64)&&pixel(5,5,0,0,0),"Integer-coordinate glyph coverage");
  std::vector<std::uint8_t> expected(W*H*4),presented(W*H*4);renderer.read_rgba(W,H,expected.data());renderer.present();renderer.read_presented_rgba(W,H,presented.data());
  check(expected==presented,"Offscreen presentation orientation");renderer.delete_texture(glyph_texture);
  renderer.clear(16,12,.25f,.5f,.75f);std::vector<std::uint8_t> resized(16*12*4),resized_presented(resized.size());
  renderer.read_rgba(16,12,resized.data());renderer.present();renderer.read_presented_rgba(16,12,resized_presented.data());
  check(resized==resized_presented&&std::abs(int(resized[0])-64)<=1&&std::abs(int(resized[1])-128)<=1&&std::abs(int(resized[2])-191)<=1,"Render-target resize and presentation");
  renderer.clear(W,H,0,0,0);
  renderer.delete_texture(texture);
  if(failed)throw std::runtime_error(report.str());
  report<<"GLES3 legacy-TL pixel checks: PASS\n";
  return report.str();
}
}
