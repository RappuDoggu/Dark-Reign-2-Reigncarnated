#include "surface_native.h"
#include "dx7/errors.h"
#include <memory>
#include <stdexcept>
#include <iostream>
#include <cstring>
using namespace NativeEngine;
void check(bool value){if(!value)throw std::runtime_error("Native surface invariant");}
struct Release {void operator()(IDirectDrawSurface7 *p)const{if(p)p->Release();}};
using Owned=std::unique_ptr<IDirectDrawSurface7,Release>;
DDSURFACEDESC2 description(unsigned w,unsigned h,unsigned depth=32){DDSURFACEDESC2 d{};d.dwSize=sizeof(d);d.dwFlags=DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT|DDSD_CAPS;d.dwWidth=w;d.dwHeight=h;d.ddsCaps.dwCaps=DDSCAPS_SYSTEMMEMORY|DDSCAPS_TEXTURE;auto &p=d.ddpfPixelFormat;p.dwSize=sizeof(p);p.dwFlags=DDPF_RGB;p.dwRGBBitCount=depth;p.dwRBitMask=depth==16?0xf800:0xff0000;p.dwGBitMask=depth==16?0x7e0:0xff00;p.dwBBitMask=depth==16?0x1f:0xff;return d;}
Owned create(const DDSURFACEDESC2 &d){IDirectDrawSurface7 *p=nullptr;check(CreateMemorySurface(d,&p)==S_OK&&p);return Owned(p);}
DDSURFACEDESC2 lock(IDirectDrawSurface7 *p,DWORD flags=0){DDSURFACEDESC2 d{};d.dwSize=sizeof(d);check(p->Lock(nullptr,&d,flags,nullptr)==S_OK);return d;}
void fill(IDirectDrawSurface7 *p,DWORD color){DDBLTFX fx{};fx.dwSize=sizeof(fx);fx.dwFillColor=color;check(p->Blt(nullptr,nullptr,nullptr,DDBLT_COLORFILL,&fx)==S_OK);}
std::vector<std::uint8_t> rgba(IDirectDrawSurface7 *p,unsigned expectedWidth,unsigned expectedHeight){std::vector<std::uint8_t> out;DWORD w=0,h=0,v=0;check(ReadSurfaceRGBA(p,out,w,h,v)==S_OK&&w==expectedWidth&&h==expectedHeight&&v);return out;}
HRESULT visitMip(IDirectDrawSurface7 *surface,DDSURFACEDESC2 *desc,void *context){
 auto &count=*static_cast<unsigned *>(context);++count;
 if(count==1)check(desc->dwWidth==2);else check(desc->dwWidth==1);
 check(surface->EnumAttachedSurfaces(context,visitMip)==S_OK);return DDENUMRET_CANCEL;
}
int main(){try{
 auto s=create(description(3,2,16));fill(s.get(),0xf800);
 auto data=rgba(s.get(),3,2);check(data[0]==255&&data[1]==0&&data[2]==0&&data[3]==255);
 DWORD before=0,after=0;s->GetUniquenessValue(&before);auto d=lock(s.get());check(d.lPitch==8);
 auto *p=static_cast<std::uint8_t *>(d.lpSurface);p[2]=0xe0;p[3]=7;p[4]=0x1f;p[5]=0;
 DDSURFACEDESC2 another{};another.dwSize=sizeof(another);check(s->Lock(nullptr,&another,0,nullptr)==DDERR_SURFACEBUSY);
 std::vector<std::uint8_t> unchanged={77};DWORD w=9,h=8,v=7;check(ReadSurfaceRGBA(s.get(),unchanged,w,h,v)==DDERR_SURFACEBUSY&&unchanged[0]==77&&w==9);
 RECT invalid={0,0,2,1};check(s->Unlock(&invalid)==DDERR_INVALIDRECT);check(s->Unlock(nullptr)==S_OK);s->GetUniquenessValue(&after);check(after!=before);
 data=rgba(s.get(),3,2);check(data[4]==0&&data[5]==255&&data[6]==0&&data[10]==255);
 before=after;lock(s.get(),DDLOCK_READONLY);check(s->Unlock(nullptr)==S_OK);s->GetUniquenessValue(&after);check(before==after&&s->Unlock(nullptr)==DDERR_NOTLOCKED);
 // Overlapping self-copy must use original source pixels.
 RECT source={0,0,2,1};check(s->BltFast(1,0,s.get(),&source,0)==S_OK);data=rgba(s.get(),3,2);check(data[0]==255&&data[4]==255&&data[8]==0&&data[9]==255);
 auto sourceSurface=create(description(2,2));auto dest=create(description(4,4));
 auto pixels=lock(sourceSurface.get());const DWORD colors[]={0xff0000,0x00ff00,0x0000ff,0xffffff};std::memcpy(pixels.lpSurface,colors,sizeof(colors));sourceSurface->Unlock(nullptr);
 fill(dest.get(),0);check(dest->Blt(nullptr,sourceSurface.get(),nullptr,0,nullptr)==S_OK);data=rgba(dest.get(),4,4);
 check(data[0]==255&&data[4]==255&&data[9]==255&&data[(2*4)*4+2]==255&&data[(3*4+3)*4]==255);
 DDCOLORKEY key{0xff0000,0xff0000};check(sourceSurface->SetColorKey(DDCKEY_SRCBLT,&key)==S_OK);fill(dest.get(),0x0000ff);
 check(dest->Blt(nullptr,sourceSurface.get(),nullptr,DDBLT_KEYSRC,nullptr)==S_OK);data=rgba(dest.get(),4,4);check(data[0]==0&&data[2]==255&&data[9]==255);
 check(dest->Blt(nullptr,sourceSurface.get(),nullptr,DDBLT_ROP,nullptr)==E_NOTIMPL);
 RECT bad={-1,0,1,1};another.dwSize=sizeof(another);check(dest->Lock(&bad,&another,0,nullptr)==DDERR_INVALIDRECT);
 check(dest->GetDC(nullptr)==E_NOTIMPL);
 void *queried=nullptr;check(dest->QueryInterface(IID_IDirectDrawSurface7,&queried)==S_OK&&queried==dest.get());static_cast<IDirectDrawSurface7 *>(queried)->Release();
 GUID wrong{};check(dest->QueryInterface(wrong,&queried)==E_NOINTERFACE&&!queried);
 auto mipDesc=description(4,4);mipDesc.dwFlags|=DDSD_MIPMAPCOUNT;mipDesc.dwMipMapCount=3;mipDesc.ddsCaps.dwCaps|=DDSCAPS_MIPMAP|DDSCAPS_COMPLEX;
 auto parent=create(mipDesc);unsigned visits=0;check(parent->EnumAttachedSurfaces(&visits,visitMip)==S_OK&&visits==2);DDSCAPS2 caps{};caps.dwCaps=DDSCAPS_MIPMAP;IDirectDrawSurface7 *childRaw=nullptr;
 check(parent->GetAttachedSurface(&caps,&childRaw)==S_OK);Owned child(childRaw);parent.reset();
 d=lock(child.get());check(d.dwWidth==2&&d.dwHeight==2);child->Unlock(nullptr);
 IDirectDrawSurface7 *lastRaw=nullptr;check(child->GetAttachedSurface(&caps,&lastRaw)==S_OK);Owned last(lastRaw);child.reset();d=lock(last.get());check(d.dwWidth==1&&d.dwHeight==1);last->Unlock(nullptr);
 auto alpha=description(1,1);alpha.ddpfPixelFormat.dwFlags|=DDPF_ALPHAPIXELS;alpha.ddpfPixelFormat.dwRGBAlphaBitMask=0xff000000;auto a=create(alpha);fill(a.get(),0x80402010);data=rgba(a.get(),1,1);check(data[0]==64&&data[1]==32&&data[2]==16&&data[3]==128);
 auto rgb24=create(description(1,2,24));d=lock(rgb24.get());check(d.lPitch==4);rgb24->Unlock(nullptr);fill(rgb24.get(),0x123456);data=rgba(rgb24.get(),1,2);check(data[0]==0x12&&data[1]==0x34&&data[2]==0x56&&data[7]==255);
 IDirectDrawSurface7 *failed=nullptr;auto invalidDesc=description(16384,16384);check(CreateMemorySurface(invalidDesc,&failed)==E_OUTOFMEMORY&&!failed);
 invalidDesc=description(4,4);invalidDesc.ddsCaps.dwCaps|=DDSCAPS_PRIMARYSURFACE;check(CreateMemorySurface(invalidDesc,&failed)==E_NOTIMPL&&!failed);
 invalidDesc=description(4,4);invalidDesc.ddpfPixelFormat.dwGBitMask=invalidDesc.ddpfPixelFormat.dwRBitMask;check(CreateMemorySurface(invalidDesc,&failed)==E_INVALIDARG&&!failed);
 std::cout<<"Native surfaces: pixel formats, locks, revisions, fill, overlap, scaling, color keys, mip ownership and bounds passed; no GPU rendering executed\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
