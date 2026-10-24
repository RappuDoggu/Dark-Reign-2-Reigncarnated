#include "vid_slice.hpp"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
// Executes original Vid setters -> native state adapter -> actual GLES -> readback.
void original_vid_gpu_checks(reign::LegacyGles &renderer){
 using namespace reign;
 constexpr unsigned W=32,H=32;
 std::array<LegacyTL,4> v={{{-.5f,-.5f,.5f,1,0x804080c0,0xff000000,0,0},{31.5f,-.5f,.5f,1,0x804080c0,0xff000000,1,0},{31.5f,31.5f,.5f,1,0x804080c0,0xff000000,1,1},{-.5f,31.5f,.5f,1,0x804080c0,0xff000000,0,1}}};
 std::uint16_t indices[]={0,1,2,0,2,3};
 VidDevice device;VidBinding binding(device);
 device.set_draw_sink([&](const LegacyTL *p,std::size_t n,const std::uint16_t *ix,std::size_t ni,const DrawState &state){renderer.draw(W,H,p,n,ix,ni,state);});
 std::vector<std::uint8_t> pixels(W*H*4);
 auto expect=[&](int r,int g,int b,int a,const char *name){
  renderer.read_rgba(W,H,pixels.data());auto p=(16*W+16)*4;
  const int expected[]={r,g,b,a};for(unsigned i=0;i<4;++i)if(std::abs(int(pixels[p+i])-expected[i])>2)throw std::runtime_error(std::string(name)+" channel "+std::to_string(i)+" got "+std::to_string(pixels[p+i])+" expected "+std::to_string(expected[i]));
  std::cout<<name<<": PASS\n";
 };
 U32 draw_flags=RS_BLEND_DEF;
 auto draw=[&]{Vid::DrawIndexedPrimitive(PT_TRIANGLELIST,FVF_TLVERTEX,v.data(),v.size(),indices,6,draw_flags);};
 const std::uint8_t rgba[]={128,64,192,128};auto id=renderer.texture_rgba(1,1,rgba);TextureView texture(id);
 const U32 flags[]={RS_TEX_DECAL,RS_TEX_DECALALPHA,RS_TEX_MODULATE,RS_TEX_MODULATE2X,RS_TEX_MODULATE4X,RS_TEX_MODULATEALPHA,RS_TEX_ADD};
 const int expected[][4]={{128,64,192,64},{64,128,192,128},{32,32,145,64},{64,64,255,64},{129,129,255,64},{32,32,145,64},{32,32,145,64}};
 for(unsigned i=0;i<7;++i){renderer.clear(W,H,0,0,0,0);Vid::SetTextureDX(&texture,0,flags[i]);draw();expect(expected[i][0],expected[i][1],expected[i][2],expected[i][3],("Original Vid texture mode "+std::to_string(i+1)).c_str());}
 Vid::SetTextureDX(nullptr,0,RS_BLEND_DEF);Vid::SetAlphaState(TRUE);
 draw_flags=RS_SRC_DSTCOLOR|RS_DST_ZERO;Vid::SetSrcBlendState(RS_SRC_DSTCOLOR);Vid::SetDstBlendState(RS_DST_ZERO);
 renderer.clear(W,H,.5f,.5f,.5f,1);draw();expect(32,64,96,128,"Original Vid destination-color blend");
 draw_flags=RS_SRC_SRCALPHA|RS_DST_ONE;Vid::SetSrcBlendState(RS_SRC_SRCALPHA);Vid::SetDstBlendState(RS_DST_ONE);
 renderer.clear(W,H,0,0,.25f,1);draw();expect(32,64,160,255,"Original Vid additive blend");
 Vid::SetAlphaState(FALSE);Vid::SetZBufferState(TRUE);renderer.clear(W,H,0,0,0,0);
 for(auto &p:v){p.z=.2f;p.diffuse=0xffff0000;}draw();
 for(auto &p:v){p.z=.8f;p.diffuse=0xff0000ff;}draw();expect(255,0,0,255,"Original Vid Z test/write");
 renderer.delete_texture(id);
 const std::uint8_t tile[]={255,0,0,255,0,255,0,255};id=renderer.texture_rgba(2,1,tile);TextureView tiled(id);
 Vid::SetZBufferState(FALSE);
 for(auto &p:v){p.diffuse=0xffffffff;p.u=1.125f;p.v=.5f;}
 const U32 address[]={0,RS_TEXMIRROR,RS_TEXCLAMP};
 for(unsigned i=0;i<3;++i){Vid::SetTextureDX(&tiled,0,RS_TEX_DECAL|address[i]);renderer.clear(W,H,0,0,0);draw();expect(i?0:255,i?255:0,0,255,("Original Vid texture address "+std::to_string(i)).c_str());}
 Vid::SetTextureDX(nullptr,0,RS_BLEND_DEF);renderer.delete_texture(id);
}
