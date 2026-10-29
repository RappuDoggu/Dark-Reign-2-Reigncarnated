#include "device_gles.h"
#include "device_unsupported.h"
#include "surface_gles.h"
#include "reign/legacy_gles.hpp"
#include <memory>
#include <vector>
#include <cmath>
#include <cstring>
#include <unordered_map>
namespace NativeEngine {
namespace {
class Device final:public UnsupportedDevice {
 ULONG refs=1;
 reign::LegacyGles &renderer;
 IDirectDrawSurface7 *target;
 IDirectDrawSurface7 *texture=nullptr;
 SurfaceTexture *upload=nullptr;
 struct CachedTexture {std::unique_ptr<SurfaceTexture> image;std::uint64_t used,bytes;};
 std::unordered_map<IDirectDrawSurface7 *,CachedTexture> textures;
 std::uint64_t textureBytes=0,textureTick=0;
 std::vector<WORD> alignedIndices,triangles,visible,sequential;
 DWORD width,height;
 bool scene=false;
 D3DMATERIAL7 material{};
 reign::DrawState state;
 DWORD colorOp=D3DTOP_MODULATE,alphaOp=D3DTOP_MODULATE;
 DWORD color1=D3DTA_TEXTURE,color2=D3DTA_DIFFUSE,alpha1=D3DTA_TEXTURE,alpha2=D3DTA_DIFFUSE;
 DWORD fogColor=0,ambientColor=0,textureFactor=0xffffffff,fogStart=0,fogEnd=0;
 bool clipping=true;
 DWORD cullMode=D3DCULL_NONE;
 DWORD minFilter=D3DTFN_POINT,magFilter=D3DTFG_POINT;

public:
 Device(reign::LegacyGles &r,IDirectDrawSurface7 *t,DWORD w,DWORD h):renderer(r),target(t),width(w),height(h){target->AddRef();}
 ~Device(){textures.clear();target->Release();}
 ULONG AddRef()override{return ++refs;}
 ULONG Release()override{auto remaining=--refs;if(!remaining)delete this;return remaining;}
 HRESULT BeginScene()override{if(scene)return E_FAIL;scene=true;return S_OK;}
 HRESULT EndScene()override{if(!scene)return E_FAIL;scene=false;return S_OK;}
 // Prelit TL vertices carry final lighting; material remains queryable DX7 state.
 HRESULT SetMaterial(D3DMATERIAL7 *value)override{if(!value)return E_INVALIDARG;material=*value;return S_OK;}
 HRESULT GetMaterial(D3DMATERIAL7 *out)override{if(!out)return E_INVALIDARG;*out=material;return S_OK;}
 HRESULT GetRenderTarget(IDirectDrawSurface7 **out)override{if(!out)return E_INVALIDARG;target->AddRef();*out=target;return S_OK;}
 HRESULT GetCaps(D3DDEVICEDESC7 *out)override{
  if(!out)return E_INVALIDARG;*out={};out->dwMaxTextureWidth=out->dwMaxTextureHeight=2048;
  out->wMaxTextureBlendStages=out->wMaxSimultaneousTextures=1;
  out->dpcTriCaps.dwTextureFilterCaps=D3DPTFILTERCAPS_MINFPOINT|D3DPTFILTERCAPS_MINFLINEAR|D3DPTFILTERCAPS_MAGFPOINT|D3DPTFILTERCAPS_MAGFLINEAR;
  return S_OK;
 }
 HRESULT EnumTextureFormats(LPD3DENUMPIXELFORMATSCALLBACK cb,void *context)override{
  if(!cb)return E_INVALIDARG;
  DDPIXELFORMAT format{};format.dwSize=sizeof(format);format.dwFlags=DDPF_RGB|DDPF_ALPHAPIXELS;format.dwRGBBitCount=32;
  format.dwRBitMask=0xff0000;format.dwGBitMask=0xff00;format.dwBBitMask=0xff;format.dwRGBAlphaBitMask=0xff000000;
  cb(&format,context);return S_OK;
 }
 HRESULT SetViewport(D3DVIEWPORT7 *v)override{
  if(!v)return E_INVALIDARG;
  return !v->dwX&&!v->dwY&&v->dwWidth==width&&v->dwHeight==height&&v->dvMinZ==0&&v->dvMaxZ==1?S_OK:E_NOTIMPL;
 }
 HRESULT GetViewport(D3DVIEWPORT7 *v)override{if(!v)return E_INVALIDARG;*v={0,0,width,height,0,1};return S_OK;}
 HRESULT Clear(DWORD count,D3DRECT *rectangles,DWORD flags,D3DCOLOR color,D3DVALUE depth,DWORD)override{
  if(count){
   if(!rectangles)return E_INVALIDARG;
   if(count!=1||rectangles[0].x1!=0||rectangles[0].y1!=0||rectangles[0].x2!=LONG(width)||rectangles[0].y2!=LONG(height))return E_NOTIMPL;
  }
  if(flags&~DWORD(D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER))return E_NOTIMPL;
  if((flags&D3DCLEAR_ZBUFFER)&&(!std::isfinite(depth)||depth<0||depth>1))return E_INVALIDARG;
  try{renderer.clear_buffers(width,height,(flags&D3DCLEAR_TARGET)!=0,(flags&D3DCLEAR_ZBUFFER)!=0,float((color>>16)&255)/255,float((color>>8)&255)/255,float(color&255)/255,float(color>>24)/255,(flags&D3DCLEAR_ZBUFFER)?depth:1);return S_OK;}catch(...){return E_FAIL;}
 }
 HRESULT SetTexture(DWORD stage,IDirectDrawSurface7 *surface)override{
  if(stage)return E_NOTIMPL;
  if(texture==surface)return S_OK;
  try{
   if(!surface){upload=nullptr;texture=nullptr;return S_OK;}
   auto found=textures.find(surface);
   if(found==textures.end()){
    DDSURFACEDESC2 desc{};desc.dwSize=sizeof(desc);if(FAILED(surface->GetSurfaceDesc(&desc)))return E_FAIL;
    const std::uint64_t bytes=std::uint64_t(desc.dwWidth)*desc.dwHeight*4;
    auto image=std::make_unique<SurfaceTexture>(surface);
    // Bound retained CPU surface references and GPU storage. LRU entries own a
    // surface reference, preventing stale pointer reuse after bitmap deletion.
    while(!textures.empty()&&(textures.size()>=512||textureBytes+bytes>64*1024*1024)){
     auto oldest=textures.begin();for(auto i=textures.begin();i!=textures.end();++i)if(i->second.used<oldest->second.used)oldest=i;
     if(upload==oldest->second.image.get()){upload=nullptr;texture=nullptr;}
     textureBytes-=oldest->second.bytes;textures.erase(oldest);
    }
    found=textures.emplace(surface,CachedTexture{std::move(image),0,bytes}).first;textureBytes+=bytes;
   }
   found->second.used=++textureTick;upload=found->second.image.get();texture=surface;return S_OK;
  }catch(...){return E_FAIL;}
 }
 HRESULT GetTexture(DWORD stage,IDirectDrawSurface7 **out)override{
  if(!out)return E_INVALIDARG;*out=nullptr;if(stage)return E_NOTIMPL;
  if(texture)texture->AddRef();*out=texture;return S_OK;
 }
 HRESULT GetTextureStageState(DWORD stage,D3DTEXTURESTAGESTATETYPE type,DWORD *out)override{
  if(!out)return E_INVALIDARG;*out=0;
  if(stage){if(stage==1&&(type==D3DTSS_COLOROP||type==D3DTSS_ALPHAOP)){*out=D3DTOP_DISABLE;return S_OK;}return E_NOTIMPL;}
  switch(type){
   case D3DTSS_COLOROP:*out=colorOp;break;case D3DTSS_ALPHAOP:*out=alphaOp;break;
   case D3DTSS_COLORARG1:*out=color1;break;case D3DTSS_COLORARG2:*out=color2;break;
   case D3DTSS_ALPHAARG1:*out=alpha1;break;case D3DTSS_ALPHAARG2:*out=alpha2;break;
   case D3DTSS_MINFILTER:*out=minFilter;break;case D3DTSS_MAGFILTER:*out=magFilter;break;
   case D3DTSS_MIPFILTER:*out=D3DTFP_NONE;break;
   case D3DTSS_ADDRESS:*out=state.address==reign::TextureAddress::wrap?D3DTADDRESS_WRAP:state.address==reign::TextureAddress::mirror?D3DTADDRESS_MIRROR:D3DTADDRESS_CLAMP;break;
   default:return E_NOTIMPL;
  }return S_OK;
 }
 HRESULT GetRenderState(D3DRENDERSTATETYPE type,DWORD *out)override{
  if(!out)return E_INVALIDARG;*out=0;
  switch(type){
   case D3DRENDERSTATE_CLIPPING:*out=clipping;break;
   case D3DRENDERSTATE_DITHERENABLE:*out=state.dither;break;
   case D3DRENDERSTATE_AMBIENT:*out=ambientColor;break;
   case D3DRENDERSTATE_TEXTUREFACTOR:*out=textureFactor;break;
   case D3DRENDERSTATE_FOGSTART:*out=fogStart;break;
   case D3DRENDERSTATE_FOGEND:*out=fogEnd;break;
   case D3DRENDERSTATE_TEXTUREPERSPECTIVE:*out=state.perspective;break;
   case D3DRENDERSTATE_ANTIALIAS:case D3DRENDERSTATE_EDGEANTIALIAS:
   case D3DRENDERSTATE_COLORKEYENABLE:case D3DRENDERSTATE_ZBIAS:*out=0;break;
   case D3DRENDERSTATE_EXTENTS:case D3DRENDERSTATE_LIGHTING:*out=0;break;
   case D3DRENDERSTATE_FILLMODE:*out=D3DFILL_SOLID;break;
   case D3DRENDERSTATE_SHADEMODE:*out=D3DSHADE_GOURAUD;break;
   case D3DRENDERSTATE_ZENABLE:*out=state.depth_test;break;
   case D3DRENDERSTATE_ZWRITEENABLE:*out=state.depth_write;break;
   case D3DRENDERSTATE_ALPHABLENDENABLE:*out=state.blend!=reign::Blend::opaque;break;
   case D3DRENDERSTATE_SRCBLEND:*out=DWORD(state.source_factor);break;
   case D3DRENDERSTATE_DESTBLEND:*out=DWORD(state.destination_factor);break;
   case D3DRENDERSTATE_FOGENABLE:*out=state.fog;break;
   case D3DRENDERSTATE_FOGCOLOR:*out=fogColor;break;
   case D3DRENDERSTATE_SPECULARENABLE:*out=state.specular;break;
   case D3DRENDERSTATE_CULLMODE:*out=cullMode;break;
   case D3DRENDERSTATE_ZFUNC:*out=D3DCMP_LESSEQUAL;break;
   case D3DRENDERSTATE_ALPHAFUNC:*out=D3DCMP_GREATER;break;
   case D3DRENDERSTATE_ALPHATESTENABLE:*out=state.alpha_test;break;
   case D3DRENDERSTATE_ALPHAREF:*out=DWORD(std::lround(state.alpha_reference*255));break;
   default:return E_NOTIMPL;
  }return S_OK;
 }
 HRESULT SetTextureStageState(DWORD stage,D3DTEXTURESTAGESTATETYPE type,DWORD value)override{
  if(stage)return (stage==1&&(type==D3DTSS_COLOROP||type==D3DTSS_ALPHAOP)&&value==D3DTOP_DISABLE)?S_OK:E_NOTIMPL;
  switch(type){
   case D3DTSS_COLOROP:case D3DTSS_ALPHAOP:
    if(value!=D3DTOP_MODULATE&&value!=D3DTOP_SELECTARG1&&!(type==D3DTSS_COLOROP&&value==D3DTOP_DISABLE)&&!(type==D3DTSS_COLOROP&&(value==D3DTOP_MODULATE2X||value==D3DTOP_MODULATE4X)))return E_NOTIMPL;
    (type==D3DTSS_COLOROP?colorOp:alphaOp)=value;return S_OK;
   case D3DTSS_COLORARG1:case D3DTSS_COLORARG2:case D3DTSS_ALPHAARG1:case D3DTSS_ALPHAARG2:
    if(value!=D3DTA_TEXTURE&&value!=D3DTA_DIFFUSE&&value!=D3DTA_CURRENT)return E_NOTIMPL;
    if(type==D3DTSS_COLORARG1)color1=value;else if(type==D3DTSS_COLORARG2)color2=value;else if(type==D3DTSS_ALPHAARG1)alpha1=value;else alpha2=value;return S_OK;
   case D3DTSS_MINFILTER:if(value!=D3DTFN_POINT&&value!=D3DTFN_LINEAR)return E_NOTIMPL;minFilter=value;return S_OK;
   case D3DTSS_MAGFILTER:if(value!=D3DTFG_POINT&&value!=D3DTFG_LINEAR)return E_NOTIMPL;magFilter=value;return S_OK;
   case D3DTSS_MIPFILTER:return value==D3DTFP_NONE?S_OK:E_NOTIMPL;
   case D3DTSS_ADDRESS:
    if(value==D3DTADDRESS_WRAP)state.address=reign::TextureAddress::wrap;
    else if(value==D3DTADDRESS_CLAMP)state.address=reign::TextureAddress::clamp;
    else if(value==D3DTADDRESS_MIRROR)state.address=reign::TextureAddress::mirror;
    else return E_NOTIMPL;return S_OK;
   default:return E_NOTIMPL;
  }
 }
 HRESULT SetRenderState(D3DRENDERSTATETYPE type,DWORD value)override{
  switch(type){
   // TL vertices are already lit; GLES clips and rasterizes solid interpolated triangles.
   case D3DRENDERSTATE_CLIPPING:if(value>1)return E_NOTIMPL;clipping=value!=0;return S_OK;
   case D3DRENDERSTATE_DITHERENABLE:if(value>1)return E_INVALIDARG;state.dither=value!=0;return S_OK;
   // TL vertices already contain lighting and the CPU-computed fog factor.
   case D3DRENDERSTATE_AMBIENT:ambientColor=value;return S_OK;
   case D3DRENDERSTATE_TEXTUREFACTOR:textureFactor=value;return S_OK;
   case D3DRENDERSTATE_FOGSTART:case D3DRENDERSTATE_FOGEND:{
    float distance;std::memcpy(&distance,&value,sizeof(distance));if(!std::isfinite(distance))return E_INVALIDARG;
    (type==D3DRENDERSTATE_FOGSTART?fogStart:fogEnd)=value;return S_OK;
   }
   case D3DRENDERSTATE_TEXTUREPERSPECTIVE:if(value>1)return E_INVALIDARG;state.perspective=value!=0;return S_OK;
   case D3DRENDERSTATE_ANTIALIAS:case D3DRENDERSTATE_EDGEANTIALIAS:
   case D3DRENDERSTATE_COLORKEYENABLE:case D3DRENDERSTATE_ZBIAS:return value==0?S_OK:E_NOTIMPL;
   case D3DRENDERSTATE_EXTENTS:case D3DRENDERSTATE_LIGHTING:return value==0?S_OK:E_NOTIMPL;
   case D3DRENDERSTATE_FILLMODE:return value==D3DFILL_SOLID?S_OK:E_NOTIMPL;
   case D3DRENDERSTATE_SHADEMODE:return value==D3DSHADE_GOURAUD?S_OK:E_NOTIMPL;
   case D3DRENDERSTATE_ZENABLE:if(value>1)return E_NOTIMPL;state.depth_test=value!=0;return S_OK;
   case D3DRENDERSTATE_ZWRITEENABLE:state.depth_write=value!=0;return S_OK;
   case D3DRENDERSTATE_ALPHABLENDENABLE:state.blend=value?reign::Blend::custom:reign::Blend::opaque;return S_OK;
   case D3DRENDERSTATE_SRCBLEND:case D3DRENDERSTATE_DESTBLEND:
    if(value<1||value>(type==D3DRENDERSTATE_SRCBLEND?11u:10u))return E_NOTIMPL;
    (type==D3DRENDERSTATE_SRCBLEND?state.source_factor:state.destination_factor)=static_cast<reign::BlendFactor>(value);return S_OK;
   case D3DRENDERSTATE_FOGENABLE:state.fog=value!=0;return S_OK;
   case D3DRENDERSTATE_FOGCOLOR:fogColor=value;for(unsigned i=0;i<3;++i)state.fog_rgb[i]=float((value>>(16-i*8))&255)/255;return S_OK;
   case D3DRENDERSTATE_SPECULARENABLE:state.specular=value!=0;return S_OK;
   case D3DRENDERSTATE_CULLMODE:if(value<D3DCULL_NONE||value>D3DCULL_CCW)return E_INVALIDARG;cullMode=value;return S_OK;
   case D3DRENDERSTATE_ZFUNC:return value==D3DCMP_LESSEQUAL?S_OK:E_NOTIMPL;
   case D3DRENDERSTATE_ALPHAFUNC:return value==D3DCMP_GREATER?S_OK:E_NOTIMPL;
   case D3DRENDERSTATE_ALPHATESTENABLE:state.alpha_test=value!=0;return S_OK;
   case D3DRENDERSTATE_ALPHAREF:if(value>255)return E_INVALIDARG;state.alpha_reference=float(value)/255;return S_OK;
   default:return E_NOTIMPL;
  }
 }
 HRESULT DrawIndexedPrimitive(D3DPRIMITIVETYPE kind,DWORD fvf,void *vertices,DWORD count,WORD *indices,DWORD n,DWORD flags)override{
  if(!scene||!vertices||!indices||!count||n<3||n>196608||count>65536)return E_INVALIDARG;
  if((kind!=D3DPT_TRIANGLELIST&&kind!=D3DPT_TRIANGLESTRIP&&kind!=D3DPT_TRIANGLEFAN)||fvf!=D3DFVF_TLVERTEX||flags)return E_NOTIMPL;
  if(kind==D3DPT_TRIANGLELIST&&n%3)return E_INVALIDARG;
  try{
   // Original bucket allocations can place the index array at an odd address.
   // Copy its bytes before accessing WORDs, including on strict-alignment ARM64.
   alignedIndices.resize(n);
   std::memcpy(alignedIndices.data(),indices,std::size_t(n)*sizeof(WORD));
   indices=alignedIndices.data();
   for(DWORD i=0;i<n;++i)if(indices[i]>=count)return E_INVALIDARG;
   triangles.clear();
   if(kind!=D3DPT_TRIANGLELIST){
    triangles.reserve(std::size_t(n-2)*3);
    for(DWORD i=2;i<n;++i){
     if(kind==D3DPT_TRIANGLEFAN)triangles.insert(triangles.end(),{indices[0],indices[i-1],indices[i]});
     else if(i&1)triangles.insert(triangles.end(),{indices[i-1],indices[i-2],indices[i]});
     else triangles.insert(triangles.end(),{indices[i-2],indices[i-1],indices[i]});
    }
    indices=triangles.data();n=DWORD(triangles.size());
   }
   auto draw=state;draw.texture_op=texture&&colorOp!=D3DTOP_DISABLE?reign::TextureOp::modulate_rgba:reign::TextureOp::diffuse;
   if(texture&&colorOp!=D3DTOP_DISABLE){
    if(color1!=D3DTA_TEXTURE||alpha1!=D3DTA_TEXTURE||
       (colorOp!=D3DTOP_SELECTARG1&&color2!=D3DTA_DIFFUSE&&color2!=D3DTA_CURRENT)||
       (alphaOp==D3DTOP_MODULATE&&alpha2!=D3DTA_DIFFUSE&&alpha2!=D3DTA_CURRENT))return E_NOTIMPL;
    if(colorOp==D3DTOP_MODULATE)draw.texture_op=alphaOp==D3DTOP_MODULATE?reign::TextureOp::modulate_rgba:reign::TextureOp::modulate_rgb_texture_alpha;
    else if(colorOp==D3DTOP_SELECTARG1)draw.texture_op=alphaOp==D3DTOP_SELECTARG1?reign::TextureOp::texture:reign::TextureOp::decal;
    else if(alphaOp==D3DTOP_MODULATE)draw.texture_op=colorOp==D3DTOP_MODULATE2X?reign::TextureOp::modulate2x:reign::TextureOp::modulate4x;
    else return E_NOTIMPL;
   }
   if((minFilter==D3DTFN_LINEAR)!=(magFilter==D3DTFG_LINEAR))return E_NOTIMPL;
   draw.filtered=minFilter==D3DTFN_LINEAR;
   draw.texture=upload?upload->Refresh(renderer):0;
   const auto *tl=static_cast<const reign::LegacyTL *>(vertices);
   visible.clear();
   if(cullMode!=D3DCULL_NONE){
    visible.reserve(n);
    for(DWORD i=0;i<n;i+=3){const auto &a=tl[indices[i]],&b=tl[indices[i+1]],&c=tl[indices[i+2]];
     // TL coordinates have y down: positive signed area is clockwise.
     const double area=(double(b.x)-a.x)*(double(c.y)-a.y)-(double(b.y)-a.y)*(double(c.x)-a.x);
     if((cullMode==D3DCULL_CW&&area>0)||(cullMode==D3DCULL_CCW&&area<0))continue;
     visible.insert(visible.end(),indices+i,indices+i+3);
    }
    if(visible.empty())return S_OK;indices=visible.data();n=DWORD(visible.size());
   }
   renderer.draw(width,height,tl,count,indices,n,draw);return S_OK;
  }catch(...){return E_FAIL;}
 }
 HRESULT DrawPrimitive(D3DPRIMITIVETYPE kind,DWORD fvf,void *vertices,DWORD count,DWORD flags)override{
  if(count>65536||!count)return E_INVALIDARG;
  try{sequential.resize(count);for(DWORD i=0;i<count;++i)sequential[i]=WORD(i);return DrawIndexedPrimitive(kind,fvf,vertices,count,sequential.data(),count,flags);}catch(...){return E_FAIL;}
 }
};
}
HRESULT CreateGlesDevice(reign::LegacyGles &renderer,IDirectDrawSurface7 *target,IDirect3DDevice7 **out){
 if(!out)return E_INVALIDARG;*out=nullptr;if(!target)return E_INVALIDARG;
 DDSURFACEDESC2 desc{};desc.dwSize=sizeof(desc);auto result=target->GetSurfaceDesc(&desc);if(FAILED(result))return result;
 if(!desc.dwWidth||!desc.dwHeight||desc.dwWidth>16384||desc.dwHeight>16384)return E_INVALIDARG;
 try{*out=new Device(renderer,target,desc.dwWidth,desc.dwHeight);return S_OK;}catch(...){return E_FAIL;}
}
}
