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
  renderer.delete_texture(texture);
  if(failed)throw std::runtime_error(report.str());
  report<<"GLES3 legacy-TL pixel checks: PASS\n";
  return report.str();
}
}
