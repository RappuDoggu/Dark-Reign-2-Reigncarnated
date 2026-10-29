#include "vid_slice.hpp"
#include <iostream>
#include <stdexcept>
using namespace reign;
void check(bool b,const char *s){if(!b)throw std::runtime_error(s);}
template<class F>void rejects(F f){try{f();}catch(const std::runtime_error&){return;}throw std::runtime_error("Unsupported state silently accepted");}
int main(){try{
 VidDevice device;VidBinding binding(device);
 check(!Vid::SetZBufferState(TRUE),"Z prior value");check(Vid::SetZWriteState(FALSE),"Z-write prior value");
 auto s=device.draw_state();check(s.depth_test&&!s.depth_write,"Z state translation");
 check(!Vid::SetAlphaState(TRUE),"Alpha prior value");
 Vid::SetSrcBlendState(RS_SRC_DSTCOLOR);Vid::SetDstBlendState(RS_DST_ZERO);
 s=device.draw_state();check(s.blend==Blend::custom&&s.source_factor==BlendFactor::destination_color&&s.destination_factor==BlendFactor::zero,"Blend translation");
 TextureView texture(9);Vid::SetTextureDX(&texture,0,RS_BLEND_DEF);
 const U32 flags[]={RS_TEX_DECAL,RS_TEX_DECALALPHA,RS_TEX_MODULATE,RS_TEX_MODULATE2X,RS_TEX_MODULATE4X,RS_TEX_MODULATEALPHA,RS_TEX_ADD};
 const TextureOp ops[]={TextureOp::decal,TextureOp::diffuse_texture_alpha,TextureOp::modulate_rgba,TextureOp::modulate2x,TextureOp::modulate4x,TextureOp::modulate_rgba,TextureOp::modulate_rgba};
 for(unsigned i=0;i<7;++i){Vid::SetTexBlendState(flags[i],0);check(device.draw_state().texture_op==ops[i],"Original texture operation");}
 check(Vid::SetTexBlendState(RS_TEX_MODULATE,0)==RS_TEX_ADD,"Prior texture flags");
 Vid::SetTexWrapState(RS_TEXCLAMP,0);check(device.draw_state().address==TextureAddress::clamp,"Clamp address");
 check(Vid::SetTexWrapState(RS_TEXMIRROR,0)==RS_TEXCLAMP,"Prior address flags");check(device.draw_state().address==TextureAddress::mirror,"Mirror address");
 Vid::SetTexWrapState(0,0);check(device.draw_state().address==TextureAddress::wrap,"Wrap address");
 Vid::TexModulate(0);check(device.draw_state().texture_op==TextureOp::modulate_rgb_texture_alpha,"Independent texture alpha");
 Vid::TexDecalSimple(0);check(device.draw_state().texture_op==TextureOp::texture,"Simple decal");
 Vid::SetTextureDX(nullptr,0,RS_BLEND_DEF);check(device.draw_state().texture==0,"Null texture unbind");
 rejects([]{Vid::SetTexBlendState(0,0);});rejects([]{Vid::TexModulate(1);});
 rejects([&]{device.SetTextureStageState(0,12,4);});rejects([&]{device.SetRenderState(22,3);});
 rejects([&]{device.SetRenderState(7,2);});rejects([&]{device.SetRenderState(19,12);});
 {VidDevice other;VidBinding nested(other);Vid::SetAlphaState(TRUE);check(Vid::device==&other,"Nested binding");}
 check(Vid::device==&device&&Vid::renderState.status.alpha,"Binding restoration");
 LegacyTL vertices[]={{0,0,.5f,1,0xffffffff,0,0,0},{8,0,.5f,1,0xffffffff,0,1,0},{0,8,.5f,1,0xffffffff,0,0,1}};
 std::uint16_t indices[]={0,1,2};
 auto draw=[&]{Vid::DrawIndexedPrimitive(PT_TRIANGLELIST,FVF_TLVERTEX,vertices,3,indices,3,RS_BLEND_DEF);};
 rejects(draw); // Missing renderer must fail rather than claim a successful draw.
 unsigned calls=0;device.set_draw_sink([&](const LegacyTL *v,std::size_t n,const std::uint16_t *ix,std::size_t ni,const DrawState &){check(v==vertices&&n==3&&ix==indices&&ni==3,"Original draw submission payload");++calls;});
 const auto prior=Vid::indexCount;draw();check(calls==1&&Vid::indexCount==prior+3,"Original draw count");
 indices[2]=99;rejects(draw);check(calls==1&&Vid::indexCount==prior+3,"Rejected draw must not submit/count");indices[2]=2;
 rejects([&]{Vid::DrawIndexedPrimitive(PT_LINESTRIP,FVF_TLVERTEX,vertices,3,indices,3,RS_BLEND_DEF);});
 rejects([&]{Vid::DrawIndexedPrimitive(PT_TRIANGLELIST,0,vertices,3,indices,3,RS_BLEND_DEF);});
 rejects([&]{Vid::DrawIndexedPrimitive(PT_TRIANGLELIST,FVF_TLVERTEX,nullptr,3,indices,3,RS_BLEND_DEF);});
 std::cout<<"Original Vid depth/blend/texture/address setters PASS; unsupported paths reject\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
