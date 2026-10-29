#include "surface_native.h"
#include "surface_unsupported.h"
#include "dx7/errors.h"
#include <atomic>
#include <cstring>
#include <algorithm>
#include <memory>
#include <limits>
namespace NativeEngine {
namespace {
constexpr GUID unknownId={0,0,0,{0xc0,0,0,0,0,0,0,0x46}};
bool sameId(REFGUID a,REFGUID b){return std::memcmp(&a,&b,sizeof(GUID))==0;}
bool channel(DWORD mask,unsigned bits){
 if(!mask)return true;
 if(bits<32&&(mask>>bits))return false;
 while(!(mask&1))mask>>=1;
 return (mask&(mask+1))==0;
}
bool formatValid(const DDPIXELFORMAT &p){
 const unsigned bits=p.dwRGBBitCount;
 if(p.dwSize!=sizeof(p)||(p.dwFlags&~(DDPF_RGB|DDPF_ALPHAPIXELS))||!(p.dwFlags&DDPF_RGB)||(bits!=16&&bits!=24&&bits!=32))return false;
 const DWORD r=p.dwRBitMask,g=p.dwGBitMask,b=p.dwBBitMask,a=p.dwRGBAlphaBitMask;
 return r&&g&&b&&!(r&g)&&!(r&b)&&!(g&b)&&!(a&(r|g|b))&&channel(r,bits)&&channel(g,bits)&&channel(b,bits)&&channel(a,bits)&&(!(p.dwFlags&DDPF_ALPHAPIXELS)||a);
}
bool sameFormat(const DDPIXELFORMAT &a,const DDPIXELFORMAT &b){return a.dwFlags==b.dwFlags&&a.dwRGBBitCount==b.dwRGBBitCount&&a.dwRBitMask==b.dwRBitMask&&a.dwGBitMask==b.dwGBitMask&&a.dwBBitMask==b.dwBBitMask&&a.dwRGBAlphaBitMask==b.dwRGBAlphaBitMask;}
bool rectangle(const RECT &r,DWORD w,DWORD h){return r.left>=0&&r.top>=0&&r.right>r.left&&r.bottom>r.top&&DWORD(r.right)<=w&&DWORD(r.bottom)<=h;}
unsigned component(DWORD value,DWORD mask){if(!mask)return 255;while(!(mask&1)){mask>>=1;value>>=1;}return unsigned((std::uint64_t(value&mask)*255+mask/2)/mask);}
class Surface final : public UnsupportedSurface {
 std::atomic<ULONG> references{1};
 DDSURFACEDESC2 desc{};
 std::vector<std::uint8_t> pixels;
 Surface *mip=nullptr;
 bool locked=false,readOnly=false,hasColorKey=false;
 RECT lockRect{};
 DDCOLORKEY colorKey{};
 DWORD revision=1,priority=0;
 unsigned bpp;
 RECT full()const{return {0,0,LONG(desc.dwWidth),LONG(desc.dwHeight)};}
 void description(DDSURFACEDESC2 &out)const{out=desc;out.lpSurface=nullptr;}
 static DWORD readPixel(const std::uint8_t *p,unsigned bytes){DWORD v=0;for(unsigned i=0;i<bytes;++i)v|=DWORD(p[i])<<(8*i);return v;}
 static void writePixel(std::uint8_t *p,unsigned bytes,DWORD v){for(unsigned i=0;i<bytes;++i)p[i]=std::uint8_t(v>>(8*i));}
public:
 Surface(const DDSURFACEDESC2 &input,unsigned levels):bpp(input.ddpfPixelFormat.dwRGBBitCount/8){
  desc.dwSize=sizeof(desc);desc.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PITCH|DDSD_PIXELFORMAT;
  desc.dwWidth=input.dwWidth;desc.dwHeight=input.dwHeight;desc.lPitch=LONG((std::uint64_t(desc.dwWidth)*bpp+3)&~3ull);desc.ddpfPixelFormat=input.ddpfPixelFormat;desc.ddsCaps=input.ddsCaps;
  if(input.dwFlags&DDSD_TEXTURESTAGE){desc.dwFlags|=DDSD_TEXTURESTAGE;desc.dwTextureStage=input.dwTextureStage;}
  pixels.resize(std::size_t(desc.lPitch)*desc.dwHeight);
  if(levels>1){desc.dwFlags|=DDSD_MIPMAPCOUNT;desc.dwMipMapCount=levels;DDSURFACEDESC2 next=desc;next.dwWidth=std::max(1u,desc.dwWidth/2);next.dwHeight=std::max(1u,desc.dwHeight/2);mip=new Surface(next,levels-1);}
 }
 ~Surface(){if(mip)mip->Release();}
 HRESULT QueryInterface(REFIID id,void **out)override{if(!out)return E_INVALIDARG;*out=nullptr;if(!sameId(id,unknownId)&&!sameId(id,IID_IDirectDrawSurface7))return E_NOINTERFACE;*out=static_cast<IDirectDrawSurface7 *>(this);AddRef();return S_OK;}
 ULONG AddRef()override{return ++references;}
 ULONG Release()override{const auto count=--references;if(!count)delete this;return count;}
 HRESULT GetSurfaceDesc(LPDDSURFACEDESC2 out)override{if(!out||out->dwSize!=sizeof(*out))return E_INVALIDARG;description(*out);return S_OK;}
 HRESULT GetPixelFormat(LPDDPIXELFORMAT out)override{if(!out||out->dwSize!=sizeof(*out))return E_INVALIDARG;*out=desc.ddpfPixelFormat;return S_OK;}
 HRESULT GetCaps(LPDDSCAPS2 out)override{if(!out)return E_INVALIDARG;*out=desc.ddsCaps;return S_OK;}
 HRESULT IsLost()override{return S_OK;}
 HRESULT Restore()override{return S_OK;}
 HRESULT Lock(LPRECT area,LPDDSURFACEDESC2 out,DWORD flags,HANDLE event)override{
  if(!out||out->dwSize!=sizeof(*out)||event)return E_INVALIDARG;
  if(flags&~(DDLOCK_WAIT|DDLOCK_READONLY|DDLOCK_WRITEONLY|DDLOCK_SURFACEMEMORYPTR|DDLOCK_NOSYSLOCK))return E_NOTIMPL;
  if((flags&DDLOCK_READONLY)&&(flags&DDLOCK_WRITEONLY))return E_INVALIDARG;
  if(locked)return DDERR_SURFACEBUSY;
  const RECT r=area?*area:full();if(!rectangle(r,desc.dwWidth,desc.dwHeight))return DDERR_INVALIDRECT;
  description(*out);out->dwWidth=r.right-r.left;out->dwHeight=r.bottom-r.top;out->dwFlags|=DDSD_LPSURFACE;out->lpSurface=pixels.data()+std::size_t(r.top)*desc.lPitch+std::size_t(r.left)*bpp;
  lockRect=r;locked=true;readOnly=(flags&DDLOCK_READONLY)!=0;return S_OK;
 }
 HRESULT Unlock(LPRECT area)override{if(!locked)return DDERR_NOTLOCKED;if(area&&std::memcmp(area,&lockRect,sizeof(RECT)))return DDERR_INVALIDRECT;locked=false;if(!readOnly)++revision;return S_OK;}
 HRESULT GetBltStatus(DWORD flags)override{if(flags!=DDGBS_CANBLT&&flags!=DDGBS_ISBLTDONE)return E_INVALIDARG;return locked?DDERR_SURFACEBUSY:S_OK;}
 HRESULT SetColorKey(DWORD flags,LPDDCOLORKEY key)override{if(flags!=DDCKEY_SRCBLT)return E_NOTIMPL;if(key&&key->dwColorSpaceLowValue>key->dwColorSpaceHighValue)return E_INVALIDARG;hasColorKey=key!=nullptr;if(key)colorKey=*key;return S_OK;}
 HRESULT GetColorKey(DWORD flags,LPDDCOLORKEY key)override{if(flags!=DDCKEY_SRCBLT)return E_NOTIMPL;if(!key)return E_INVALIDARG;if(!hasColorKey)return DDERR_COLORKEYNOTSET;*key=colorKey;return S_OK;}
 HRESULT GetAttachedSurface(LPDDSCAPS2 caps,LPDIRECTDRAWSURFACE7 *out)override{if(!out||!caps)return E_INVALIDARG;*out=nullptr;if(!mip||(mip->desc.ddsCaps.dwCaps&caps->dwCaps)!=caps->dwCaps||caps->dwCaps2||caps->dwCaps3||caps->dwCaps4)return DDERR_NOTFOUND;mip->AddRef();*out=mip;return S_OK;}
 HRESULT EnumAttachedSurfaces(LPVOID context,LPDDENUMSURFACESCALLBACK7 callback)override{
  if(!callback)return E_INVALIDARG;if(!mip)return S_OK;
  struct Keep {Surface *p;~Keep(){p->Release();}} keep{mip};mip->AddRef();
  DDSURFACEDESC2 info{};mip->description(info);
  const auto result=callback(mip,&info,context);
  return result==DDENUMRET_OK||result==DDENUMRET_CANCEL?S_OK:result;
 }
 HRESULT GetUniquenessValue(LPDWORD out)override{if(!out)return E_INVALIDARG;*out=revision;return S_OK;}
 HRESULT ChangeUniquenessValue()override{++revision;return S_OK;}
 HRESULT SetPriority(DWORD value)override{priority=value;return S_OK;}
 HRESULT GetPriority(LPDWORD out)override{if(!out)return E_INVALIDARG;*out=priority;return S_OK;}
 HRESULT SetLOD(DWORD value)override{return value?E_NOTIMPL:S_OK;}
 HRESULT GetLOD(LPDWORD out)override{if(!out)return E_INVALIDARG;*out=0;return S_OK;}
 HRESULT Blt(LPRECT destination,LPDIRECTDRAWSURFACE7 source,LPRECT origin,DWORD flags,LPDDBLTFX fx)override{
  if(flags&~(DDBLT_WAIT|DDBLT_COLORFILL|DDBLT_KEYSRC))return E_NOTIMPL;
  if(locked)return DDERR_SURFACEBUSY;
  RECT d=destination?*destination:full();if(!rectangle(d,desc.dwWidth,desc.dwHeight))return DDERR_INVALIDRECT;
  if(flags&DDBLT_COLORFILL){
   if(source||origin||!fx||fx->dwSize!=sizeof(*fx)||(flags&DDBLT_KEYSRC))return E_INVALIDARG;
   for(LONG y=d.top;y<d.bottom;++y)for(LONG x=d.left;x<d.right;++x)writePixel(pixels.data()+std::size_t(y)*desc.lPitch+std::size_t(x)*bpp,bpp,fx->dwFillColor);
  }else{
   auto *s=dynamic_cast<Surface *>(source);if(!s)return E_INVALIDARG;if(s->locked)return DDERR_SURFACEBUSY;
   if(!sameFormat(desc.ddpfPixelFormat,s->desc.ddpfPixelFormat))return DDERR_INVALIDPIXELFORMAT;
   RECT r=origin?*origin:s->full();if(!rectangle(r,s->desc.dwWidth,s->desc.dwHeight))return DDERR_INVALIDRECT;
   if((flags&DDBLT_KEYSRC)&&!s->hasColorKey)return DDERR_COLORKEYNOTSET;
   // Snapshot self-copies so overlap has deterministic source pixels.
   std::vector<std::uint8_t> snapshot;const auto *data=s->pixels.data();
   if(s==this){try{snapshot=pixels;}catch(const std::bad_alloc &){return E_OUTOFMEMORY;}data=snapshot.data();}
   for(LONG y=d.top;y<d.bottom;++y)for(LONG x=d.left;x<d.right;++x){
    const auto sy=r.top+std::int64_t(y-d.top)*(r.bottom-r.top)/(d.bottom-d.top),sx=r.left+std::int64_t(x-d.left)*(r.right-r.left)/(d.right-d.left);
    const auto value=readPixel(data+std::size_t(sy)*s->desc.lPitch+std::size_t(sx)*bpp,bpp);
    if((flags&DDBLT_KEYSRC)&&value>=s->colorKey.dwColorSpaceLowValue&&value<=s->colorKey.dwColorSpaceHighValue)continue;
    writePixel(pixels.data()+std::size_t(y)*desc.lPitch+std::size_t(x)*bpp,bpp,value);
   }
  }
  ++revision;return S_OK;
 }
 HRESULT BltFast(DWORD x,DWORD y,LPDIRECTDRAWSURFACE7 source,LPRECT origin,DWORD flags)override{
  if(flags&~(DDBLTFAST_WAIT|DDBLTFAST_SRCCOLORKEY))return E_NOTIMPL;
  auto *s=dynamic_cast<Surface *>(source);if(!s)return E_INVALIDARG;RECT r=origin?*origin:s->full();
  if(!rectangle(r,s->desc.dwWidth,s->desc.dwHeight)||std::uint64_t(x)+r.right-r.left>desc.dwWidth||std::uint64_t(y)+r.bottom-r.top>desc.dwHeight)return DDERR_INVALIDRECT;
  RECT d={LONG(x),LONG(y),LONG(x+r.right-r.left),LONG(y+r.bottom-r.top)};
  return Blt(&d,source,&r,((flags&DDBLTFAST_WAIT)?DDBLT_WAIT:0)|((flags&DDBLTFAST_SRCCOLORKEY)?DDBLT_KEYSRC:0),nullptr);
 }
 HRESULT rgba(std::vector<std::uint8_t> &out,DWORD &w,DWORD &h,DWORD &version){
  if(locked)return DDERR_SURFACEBUSY;
  try{std::vector<std::uint8_t> converted(std::size_t(desc.dwWidth)*desc.dwHeight*4);
   const auto &f=desc.ddpfPixelFormat;
   for(DWORD y=0;y<desc.dwHeight;++y)for(DWORD x=0;x<desc.dwWidth;++x){auto value=readPixel(pixels.data()+std::size_t(y)*desc.lPitch+std::size_t(x)*bpp,bpp);auto *p=converted.data()+(std::size_t(y)*desc.dwWidth+x)*4;p[0]=component(value,f.dwRBitMask);p[1]=component(value,f.dwGBitMask);p[2]=component(value,f.dwBBitMask);p[3]=(f.dwFlags&DDPF_ALPHAPIXELS)?component(value,f.dwRGBAlphaBitMask):255;}
   out.swap(converted);w=desc.dwWidth;h=desc.dwHeight;version=revision;return S_OK;
  }catch(const std::bad_alloc &){return E_OUTOFMEMORY;}
 }
};
}
HRESULT CreateMemorySurface(const DDSURFACEDESC2 &desc,IDirectDrawSurface7 **out){
 if(!out)return E_INVALIDARG;*out=nullptr;
 if(desc.dwSize!=sizeof(desc)||!desc.dwWidth||!desc.dwHeight||desc.dwWidth>16384||desc.dwHeight>16384||!formatValid(desc.ddpfPixelFormat))return E_INVALIDARG;
 const DWORD supportedCaps=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY|DDSCAPS_OFFSCREENPLAIN|DDSCAPS_MIPMAP|DDSCAPS_COMPLEX;
 if((desc.ddsCaps.dwCaps&~supportedCaps)||(desc.ddsCaps.dwCaps2&~(DDSCAPS2_TEXTUREMANAGE|DDSCAPS2_D3DTEXTUREMANAGE|DDSCAPS2_HINTDYNAMIC))||desc.ddsCaps.dwCaps3||desc.ddsCaps.dwCaps4)return E_NOTIMPL;
 if((desc.dwFlags&(DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT))!=(DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT))return E_INVALIDARG;
 if(desc.dwFlags&~(DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT|DDSD_MIPMAPCOUNT|DDSD_TEXTURESTAGE))return E_NOTIMPL;
 if((desc.dwFlags&DDSD_TEXTURESTAGE)&&desc.dwTextureStage>7)return E_INVALIDARG;
 unsigned maximum=1;for(DWORD w=desc.dwWidth,h=desc.dwHeight;w>1||h>1;++maximum){w=std::max(1u,w/2);h=std::max(1u,h/2);}
 const unsigned levels=(desc.dwFlags&DDSD_MIPMAPCOUNT)?desc.dwMipMapCount:1;
 if(!levels||levels>maximum||(levels>1&&!(desc.ddsCaps.dwCaps&DDSCAPS_MIPMAP)))return E_INVALIDARG;
 std::uint64_t total=0;DWORD w=desc.dwWidth,h=desc.dwHeight;
 for(unsigned i=0;i<levels;++i){total+=((std::uint64_t(w)*(desc.ddpfPixelFormat.dwRGBBitCount/8)+3)&~3ull)*h;w=std::max(1u,w/2);h=std::max(1u,h/2);}
 if(total>256u*1024u*1024u)return E_OUTOFMEMORY;
 try{*out=new Surface(desc,levels);return S_OK;}catch(const std::bad_alloc &){return E_OUTOFMEMORY;}
}
HRESULT ReadSurfaceRGBA(IDirectDrawSurface7 *surface,std::vector<std::uint8_t> &out,DWORD &w,DWORD &h,DWORD &revision){auto *s=dynamic_cast<Surface *>(surface);return s?s->rgba(out,w,h,revision):E_INVALIDARG;}
}
