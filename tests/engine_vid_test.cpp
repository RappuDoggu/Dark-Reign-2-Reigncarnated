#include "vid_private.h"
#include "sound.h"
#include "music_native.h"
#include "vid_native.h"
#include "surface_native.h"
#include "device_unsupported.h"
#include <stdexcept>
#include <cstdio>
#define check(b) do { if (!(b)) throw std::runtime_error("Native Vid boundary check failed at line " + std::to_string(__LINE__)); } while (0)
struct Device : NativeEngine::UnsupportedDevice {
 ULONG references=1; IDirectDrawSurface7 *target; bool rejectViewport=false, emptyFormats=false;
 D3DVIEWPORT7 viewport{};
 explicit Device(IDirectDrawSurface7 *s):target(s){target->AddRef();}
 ~Device(){target->Release();}
 ULONG AddRef()override{return ++references;}
 ULONG Release()override{return --references;} // Stack-owned test fixture.
 HRESULT GetRenderTarget(IDirectDrawSurface7 **out)override{if(!out)return E_INVALIDARG;target->AddRef();*out=target;return S_OK;}
 HRESULT GetCaps(D3DDEVICEDESC7 *out)override{*out={};out->dwMaxTextureWidth=out->dwMaxTextureHeight=1024;return S_OK;}
 HRESULT SetViewport(D3DVIEWPORT7 *v)override{if(rejectViewport)return E_FAIL;viewport=*v;return S_OK;}
 HRESULT EnumTextureFormats(LPD3DENUMPIXELFORMATSCALLBACK cb,void *ctx)override{
  if(emptyFormats)return S_OK;
  DDPIXELFORMAT p{};p.dwSize=sizeof(p);target->GetPixelFormat(&p);cb(&p,ctx);return S_OK;
 }
};
struct Presentation { int calls=0; bool success=true; };
static bool present(void *p){auto &v=*static_cast<Presentation *>(p);++v.calls;return v.success;}
int main(){try{
 using namespace NativeEngine;
 struct GuardedStatus { Vid::Status status; U32 canary = 0x12345678; } guarded;
 guarded.status.active = TRUE; guarded.status.ClearData();
 check(!guarded.status.active && guarded.canary == 0x12345678);
 check(AttachVideo(nullptr,nullptr,nullptr,nullptr)==E_INVALIDARG);
 check(!Vid::RenderFlush());
 bool rejected=false;try{Vid::PixForm(0);}catch(const std::logic_error&){rejected=true;}check(rejected);
 DDSURFACEDESC2 desc{};desc.dwSize=sizeof(desc);desc.dwWidth=64;desc.dwHeight=32;
 desc.dwFlags=DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT|DDSD_CAPS;
 desc.ddsCaps.dwCaps=DDSCAPS_SYSTEMMEMORY|DDSCAPS_OFFSCREENPLAIN;
 auto &pf=desc.ddpfPixelFormat;pf.dwSize=sizeof(pf);pf.dwFlags=DDPF_RGB;
 pf.dwRGBBitCount=16;pf.dwRBitMask=0xf800;pf.dwGBitMask=0x7e0;pf.dwBBitMask=0x1f;
 IDirectDrawSurface7 *surface=nullptr;check(CreateMemorySurface(desc,&surface)==S_OK);
 {
 Device d(surface);Presentation p;
 d.emptyFormats=true;check(AttachVideo(&d,surface,present,&p)==E_NOTIMPL);
 check(!Vid::device&&!Vid::backBmp.GetSurface()&&d.references==1);
 d.emptyFormats=false;d.rejectViewport=true;
 check(AttachVideo(&d,surface,present,&p)==E_FAIL);check(!Vid::device);
 d.rejectViewport=false;
 check(AttachVideo(&d,surface,present,&p)==S_OK);check(d.references==2);
 check(Vid::viewRect.Width()==64&&Vid::viewRect.Height()==32);
 check(d.viewport.dwWidth==64&&d.viewport.dwHeight==32);
 check(Vid::caps.maxTexWid==1024&&Vid::caps.maxTexHgt==1024);
 check(Vid::PixForm(500).pixFmt.dwRBitMask==0xf800);
 check(Vid::backBmp.GetSurface()==surface);
 check(AttachVideo(&d,surface,present,&p)==E_FAIL);
 check(Vid::RenderFlush()&&p.calls==1);
 p.success=false;check(!Vid::RenderFlush()&&p.calls==2);
 DetachVideo();check(d.references==1&&!Vid::device&&!Vid::backBmp.GetSurface());
 check(!Vid::RenderFlush()&&p.calls==2);
 DetachVideo();check(d.references==1);
 }
 check(surface->Release()==0);
 std::puts("Native Vid attachment, format selection, presentation failure and cleanup pass; fixture device only, no mission rendered.");
 return 0;
 }catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
