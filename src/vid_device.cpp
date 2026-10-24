#include "reign/vid_prelude.hpp"
#include <cmath>
#include <utility>
namespace Vid {
thread_local PortableRenderState renderState;
thread_local reign::VidDevice *device=nullptr;
thread_local int dxError=0;
thread_local U32 indexCount=0;
}
namespace reign {
VidBinding::VidBinding(VidDevice &d):previous_(Vid::device),previous_state_(Vid::renderState),previous_error_(Vid::dxError){
 Vid::device=&d;Vid::renderState={};Vid::dxError=0;
 U32 value=0;d.GetRenderState(7,&value);Vid::renderState.status.zbuffer=value;
 d.GetRenderState(27,&value);Vid::renderState.status.alpha=value;
 d.GetRenderState(19,&value);Vid::renderState.renderFlags=value<<16;
 d.GetRenderState(20,&value);Vid::renderState.renderFlags|=value<<20;
}
VidBinding::~VidBinding(){Vid::device=previous_;Vid::renderState=previous_state_;Vid::dxError=previous_error_;}
VidDevice::VidDevice(){render_[19]=5;render_[20]=6;stage_[1]=4;stage_[2]=2;stage_[3]=0;stage_[4]=4;stage_[5]=2;stage_[6]=0;stage_[12]=1;}
int VidDevice::SetRenderState(U32 k,U32 v){
 switch(k){case 7:case 14:case 27:case 28:case 136:if(v>1)throw std::runtime_error("Unsupported Vid boolean/W-buffer state");break;
 case 34:break;
 case 19:case 20:if(v<1||v>(k==20?10u:11u))throw std::runtime_error("Unsupported Vid blend factor");break;
 default:throw std::runtime_error("Unsupported Vid render state");}
 render_[k]=v;return 0;
}
int VidDevice::GetRenderState(U32 k,U32 *v)const{
 if(!v||(k!=7&&k!=14&&k!=19&&k!=20&&k!=27&&k!=28&&k!=34&&k!=136))throw std::runtime_error("Unsupported Vid render-state query");
 *v=render_[k];return 0;
}
int VidDevice::SetTextureStageState(U32 stage,U32 k,U32 v){
 if(stage==1&&(k==1||k==4)&&v==1)return 0; // Explicit termination of the one supported texture stage.
 if(stage!=0)throw std::runtime_error("Multitexture Vid stages not implemented");
 if(k==12){if(v<1||v>3)throw std::runtime_error("Unsupported Vid texture address");}
 else if(k==1||k==4){if(v<2||v>8)throw std::runtime_error("Unsupported Vid texture operation");}
 else if(k==2||k==3||k==5||k==6){if(v>2)throw std::runtime_error("Unsupported Vid texture argument");}
 else throw std::runtime_error("Unsupported Vid texture-stage state");
 stage_[k]=v;return 0;
}
void VidDevice::set_draw_sink(Sink sink){sink_=std::move(sink);}
int VidDevice::DrawIndexedPrimitive(U32 primitive,U32 fvf,void *data,U32 count,std::uint16_t *indices,U32 index_count,U32 flags){
 if(primitive!=PT_TRIANGLELIST||fvf!=FVF_TLVERTEX||flags>1)throw std::runtime_error("Unsupported Vid primitive/FVF/flags");
 if(!data||!indices||reinterpret_cast<std::uintptr_t>(data)%alignof(LegacyTL)||reinterpret_cast<std::uintptr_t>(indices)%alignof(std::uint16_t)||!count||count>65536||!index_count||index_count%3||index_count>0x7fffffffu)throw std::runtime_error("Invalid Vid batch bounds");
 const auto vertices=static_cast<const LegacyTL *>(data);
 for(U32 i=0;i<count;++i){const auto &v=vertices[i];if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z)||!std::isfinite(v.rhw)||v.rhw<=0||!std::isfinite(v.u)||!std::isfinite(v.v))throw std::runtime_error("Invalid Vid vertex");}
 for(U32 i=0;i<index_count;++i)if(indices[i]>=count)throw std::runtime_error("Invalid Vid index");
 if(!sink_)throw std::runtime_error("Vid renderer sink not bound");
 sink_(vertices,count,indices,index_count,draw_state());return 0;
}
int VidDevice::SetTexture(U32 stage,unsigned texture){
 if(stage)throw std::runtime_error("Multitexture Vid binding not implemented");
 texture_=texture;return 0;
}
DrawState VidDevice::draw_state()const{
 const auto texture=texture_;
 DrawState s;s.texture=texture;s.depth_test=render_[7];s.depth_write=render_[14];
 s.fog=render_[28];s.fog_rgb[0]=float((render_[34]>>16)&255)/255;s.fog_rgb[1]=float((render_[34]>>8)&255)/255;s.fog_rgb[2]=float(render_[34]&255)/255;
 s.blend=render_[27]?Blend::custom:Blend::opaque;
 s.source_factor=static_cast<BlendFactor>(render_[19]);s.destination_factor=static_cast<BlendFactor>(render_[20]);
 s.address=stage_[12]==1?TextureAddress::wrap:stage_[12]==2?TextureAddress::mirror:TextureAddress::clamp;
 if(!texture)return s;
 // Stage-zero CURRENT and DIFFUSE have the same initial value.
 auto arg=[&](U32 key){return stage_[key]==1?0:stage_[key];};
 const auto color=stage_[1],alpha=stage_[4];
 if(arg(2)==2 && color==4 && arg(3)==0 && arg(5)==2){
  if(alpha==4&&arg(6)==0)s.texture_op=TextureOp::modulate_rgba;
  else if(alpha==2)s.texture_op=TextureOp::modulate_rgb_texture_alpha;
  else throw std::runtime_error("Unsupported Vid alpha combiner");
 }else if(arg(2)==2&&(color==5||color==6)&&arg(3)==0&&alpha==4&&arg(5)==2&&arg(6)==0){
  s.texture_op=color==5?TextureOp::modulate2x:TextureOp::modulate4x;
 }else if(color==2&&arg(2)==2&&arg(5)==2){
  if(alpha==4&&arg(6)==0)s.texture_op=TextureOp::decal;
  else if(alpha==2)s.texture_op=TextureOp::texture;
  else throw std::runtime_error("Unsupported Vid decal alpha");
 }else if(color==2&&arg(2)==0&&alpha==2&&arg(5)==2)s.texture_op=TextureOp::diffuse_texture_alpha;
 else throw std::runtime_error("Unsupported Vid stage-zero combiner");
 return s;
}
}
