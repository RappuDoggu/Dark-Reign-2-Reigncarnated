#include "reign/renderer_checks.hpp"
#include "reign/mesh_address.hpp"
#include <array>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <limits>
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
  auto tiled=white;for(auto &v:tiled)v.u*=2;
  state.texture_op=TextureOp::texture;state.address=mesh_address(0);renderer.clear(W,H,0,0,0);draw(tiled,state);check(pixel(20,8,255,0,0)&&pixel(28,8,0,255,0),"GOD default wrap repeats UVs beyond one");
  state.address=mesh_address(1u<<14);renderer.clear(W,H,0,0,0);draw(tiled,state);check(pixel(20,8,0,255,0)&&pixel(28,8,255,0,0),"GOD mirror address flag");
  state.address=mesh_address(2u<<14);renderer.clear(W,H,0,0,0);draw(tiled,state);check(pixel(20,8,0,255,0)&&pixel(28,8,0,255,0),"GOD explicit clamp address flag");
  // Minify a one-texel black/white checker to its averaged mip. Updating the
  // retained top level must invalidate derived mips without another upload.
  std::vector<std::uint8_t> checker(64*64*4,255);for(unsigned y=0;y<64;++y)for(unsigned x=0;x<64;++x)for(unsigned c=0;c<3;++c)checker[(y*64+x)*4+c]=(x+y)%2?255:0;
  auto mipTexture=renderer.texture_rgba(64,64,checker.data());auto minified=white;for(auto &v:minified){v.u*=8;v.v*=8;}
  state={};state.texture=mipTexture;state.texture_op=TextureOp::texture;state.filtered=true;state.address=TextureAddress::wrap;state.mip_filter=MipFilter::linear;
  renderer.clear(W,H,0,0,0);draw(minified,state);check(pixel(16,16,128,128,128),"Trilinear minification averages retained mip chain");
  for(unsigned i=0;i<64*64;++i){checker[i*4]=255;checker[i*4+1]=checker[i*4+2]=0;}renderer.update_texture_rgba(mipTexture,64,64,checker.data());renderer.clear(W,H,0,0,0);draw(minified,state);check(pixel(16,16,255,0,0),"Retained texture updates invalidate derived mip levels");renderer.delete_texture(mipTexture);
  state={};state.blend=reign::Blend::alpha;auto alpha=quad(0x80ff0000);renderer.clear(W,H,0,0,1);draw(alpha,state);check(pixel(16,16,128,0,127),"Source-alpha blending");
  state.alpha_test=true;state.alpha_reference=.75f;renderer.clear(W,H,0,0,1);draw(alpha,state);check(pixel(16,16,0,0,255),"Alpha reference discard");
  state={};state.depth_test=true;state.depth_write=true;auto near=quad(0xffff0000,.2f),far=quad(0xff0000ff,.8f);renderer.clear(W,H,0,0,0);draw(near,state);draw(far,state);check(pixel(16,16,255,0,0),"Zero-to-one depth mapping and occlusion");
  state={};state.fog=true;state.fog_rgb[1]=1;auto fog=quad(0xffff0000,.5f,0);renderer.clear(W,H,0,0,0);draw(fog,state);check(pixel(16,16,0,255,0),"Specular-alpha vertex fog");
  state={};state.specular=true;auto spec=quad(0xff800000,.5f,0xff000040);renderer.clear(W,H,0,0,0);draw(spec,state);check(pixel(16,16,128,0,64),"Specular RGB addition");
  state={};auto rhs=quad(0xffff0000);rhs[1].rhw=.25f;rhs[2].rhw=.5f;renderer.clear(W,H,0,0,0);draw(rhs,state);check(pixel(16,16,255,0,0),"Reciprocal homogeneous W path");
  bool rejected=false;const std::uint16_t bad[]={0,1,99};try{renderer.draw(W,H,red.data(),red.size(),bad,3,state);}catch(const std::exception &){rejected=true;}check(rejected,"Invalid index rejection");
  // Indexed draws may have an unused capacity tail. Poisoned unused data
  // must not reject valid referenced geometry; invalid referenced data still fails.
  std::vector<LegacyTL> poisoned(red.begin(),red.end());poisoned.push_back(red[0]);poisoned.back().rhw=std::numeric_limits<float>::quiet_NaN();
  renderer.clear(W,H,0,0,0);renderer.draw(W,H,poisoned.data(),poisoned.size(),indices,6,state);check(pixel(16,16,255,0,0),"Unused poisoned TL vertex does not reject referenced geometry");
  const std::uint16_t poisonedIndices[]={0,1,4};rejected=false;
  try{renderer.draw(W,H,poisoned.data(),poisoned.size(),poisonedIndices,3,state);}catch(const std::exception &){rejected=true;}check(rejected,"Referenced poisoned TL vertex is rejected");
  rejected=false;try{renderer.draw(W,H,poisoned.data(),poisoned.size(),bad,3,state);}catch(const std::exception &e){rejected=std::string(e.what()).find("index out of bounds")!=std::string::npos;}check(rejected,"Index bounds validated before poisoned vertex coordinates");
  // Homogeneous world batches must match the already-projected original TL path.
  auto varied=quad(0xffffffff);varied[1].rhw=.4f;varied[2].rhw=.6f;varied[3].rhw=.8f;
  varied[0].diffuse=0xffff0000;varied[1].diffuse=0xff00ff00;varied[2].diffuse=0xff0000ff;
  state={};state.texture=texture;state.texture_op=TextureOp::modulate_rgba;
  renderer.clear(W,H,0,0,0);draw(varied,state);std::vector<std::uint8_t> tlPixels(W*H*4);renderer.read_rgba(W,H,tlPixels.data());
  std::array<LegacyClip,4> homogeneous{};
  for(unsigned i=0;i<4;++i){auto &v=varied[i];float w=1/v.rhw;homogeneous[i]={(v.x*2/W-1)*w,(v.y*2/H-1)*w,v.z*w,w,v.diffuse,v.specular,v.u,v.v};}
  renderer.clear(W,H,0,0,0);renderer.draw_clip(W,H,homogeneous.data(),4,indices,6,state);renderer.read_rgba(W,H,pixels.data());
  bool same=true;for(std::size_t i=0;i<pixels.size();++i)if(std::abs(int(pixels[i])-int(tlPixels[i]))>2)same=false;
  check(same,"World/TL perspective texture and affine color parity");
  const std::uint16_t triangle[]={0,1,2};
  std::vector<LegacyClip> spare(homogeneous.begin(),homogeneous.end());spare.push_back(homogeneous[0]);spare.back().w=std::numeric_limits<float>::quiet_NaN();
  renderer.clear(W,H,0,0,0);renderer.draw_clip(W,H,spare.data(),spare.size(),indices,6,state);
  rejected=false;try{renderer.draw_clip(W,H,spare.data(),spare.size(),poisonedIndices,3,state);}catch(const std::exception &){rejected=true;}check(rejected,"Referenced invalid homogeneous vertex rejected; unused invalid vertex allowed");
  std::array<LegacyClip,3> crossing={{{-.8f,-.8f,.5f,1,0xffff0000,0xff000000,0,0},{.8f,-.8f,.5f,1,0xffff0000,0xff000000,0,0},{0,.8f,-.5f,1,0xffff0000,0xff000000,0,0}}};
  state={};state.depth_test=true;state.depth_write=true;renderer.clear(W,H,0,0,0);renderer.draw_clip(W,H,crossing.data(),3,triangle,3,state);
  check(pixel(16,8,255,0,0)&&pixel(16,24,0,0,0),"World near-plane triangle clipping");
  for(auto &v:crossing){v.w=-1;v.z=-.5f;}
  renderer.clear(W,H,0,0,0);renderer.draw_clip(W,H,crossing.data(),3,triangle,3,state);
  check(pixel(16,8,0,0,0)&&pixel(16,24,0,0,0),"Behind-camera world geometry rejected by GPU clipping");
  crossing[0]={-.8f,-.8f,.5f,1,0xffff0000,0xff000000,0,0};crossing[1]={.8f,-.8f,.5f,1,0xffff0000,0xff000000,0,0};crossing[2]={0,1.2f,-1,-1,0xffff0000,0xff000000,0,0};
  renderer.clear(W,H,0,0,0);renderer.draw_clip(W,H,crossing.data(),3,triangle,3,state);
  check(pixel(16,5,255,0,0)&&pixel(0,31,0,0,0),"World triangle crossing W=0 stays bounded");
  for(auto &v:homogeneous){v.diffuse=0xff0000ff;v.z=.8f*v.w;}
  state={};state.depth_test=true;state.depth_write=true;renderer.clear(W,H,0,0,0);draw(near,state);renderer.draw_clip(W,H,homogeneous.data(),4,indices,6,state);
  check(pixel(16,16,255,0,0),"World and TL batches share zero-to-one depth");
  state={};renderer.clear(W,H,0,0,0);draw(red,state);check(pixel(16,16,255,0,0),"TL UI state restored after world draw");
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
  LegacyTL line[]={{4,8,.5f,1,0xffff0000,0xff000000,0,0},{28,8,.5f,1,0xffff0000,0xff000000,0,0}};std::uint16_t lineIndices[]={0,1};state={};renderer.clear(W,H,0,0,0);renderer.draw_lines(W,H,line,2,lineIndices,2,state);check(pixel(16,8,255,0,0),"TL line list renders expected colored pixels");
  const bool previousMode=renderer.synchronized_stream();
  for(bool synchronized:{false,true}){
   report<<"Rollover mode: "<<(synchronized?"synchronized":"fast")<<'\n';renderer.synchronized_stream(synchronized);
   for(unsigned frame=0;frame<3;++frame){
    auto left=quad(0xffff0000);for(auto &v:left)if(v.x>0)v.x=W/2.f-.5f;
    auto right=quad(0xff00ff00);for(auto &v:right)if(v.x<0)v.x=W/2.f-.5f;
    std::vector<LegacyTL> large(65536,right[0]);for(unsigned i=0;i<4;++i)large[i]=right[i];
    std::vector<std::uint16_t> many(150000);for(unsigned i=0;i<many.size();++i)many[i]=indices[i%6];
    renderer.clear(W,H,0,0,0);auto before=renderer.statistics();
    state={};draw(left,state);renderer.draw(W,H,large.data(),large.size(),many.data(),many.size(),state);
    // No readback/finish between submissions: old queued geometry must survive orphaning.
    check(pixel(8,16,255,0,0)&&pixel(24,16,0,255,0),"Append-only arena rollover preserves pending red/green draws");
    check(renderer.statistics().stream_renewals>before.stream_renewals,"Arena renewal telemetry");renderer.present();
   }
  }
  renderer.synchronized_stream(previousMode);

  if(failed)throw std::runtime_error(report.str());
  report<<"GLES3 legacy-TL pixel checks: PASS\n";
  return report.str();
}
}
