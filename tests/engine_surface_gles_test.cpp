#include "surface_gles.h"
#include "device_gles.h"
#include "reign/legacy_gles.hpp"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <memory>
namespace {
constexpr unsigned W=32,H=32;
void *resolve(const char *name){return reinterpret_cast<void *>(eglGetProcAddress(name));}
struct Context {
 EGLDisplay display=EGL_NO_DISPLAY;EGLSurface surface=EGL_NO_SURFACE;EGLContext context=EGL_NO_CONTEXT;
 Context(){
  auto platform=reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
  if(!platform)throw std::runtime_error("No EGL platform entry point");
  display=platform(EGL_PLATFORM_SURFACELESS_MESA,nullptr,nullptr);
  if(display==EGL_NO_DISPLAY||!eglInitialize(display,nullptr,nullptr)||!eglBindAPI(EGL_OPENGL_ES_API))throw std::runtime_error("Cannot initialize software EGL");
  EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT_KHR,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,16,EGL_NONE};
  EGLConfig config;EGLint count=0;if(!eglChooseConfig(display,attrs,&config,1,&count)||count!=1)throw std::runtime_error("No GLES3 config");
  EGLint pbuffer[]={EGL_WIDTH,W,EGL_HEIGHT,H,EGL_NONE};surface=eglCreatePbufferSurface(display,config,pbuffer);
  EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};context=eglCreateContext(display,config,EGL_NO_CONTEXT,ca);
  if(surface==EGL_NO_SURFACE||context==EGL_NO_CONTEXT||!eglMakeCurrent(display,surface,surface,context))throw std::runtime_error("Cannot create GLES3 context");
 }
 ~Context(){eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);if(context!=EGL_NO_CONTEXT)eglDestroyContext(display,context);if(surface!=EGL_NO_SURFACE)eglDestroySurface(display,surface);if(display!=EGL_NO_DISPLAY)eglTerminate(display);}
};
std::array<reign::LegacyTL,4> quad(std::uint32_t color,float z=.5f,std::uint32_t spec=0xff000000u){return {{{-.5f,-.5f,z,1,color,spec,0,0},{W-.5f,-.5f,z,1,color,spec,1,0},{W-.5f,H-.5f,z,1,color,spec,1,1},{-.5f,H-.5f,z,1,color,spec,0,1}}};}
const std::uint16_t indices[]={0,1,2,0,2,3};
}

void check(bool ok){if(!ok)throw std::runtime_error("Native surface GLES invariant");}
void fill(IDirectDrawSurface7 *s,DWORD color){DDBLTFX fx{};fx.dwSize=sizeof(fx);fx.dwFillColor=color;check(s->Blt(nullptr,nullptr,nullptr,DDBLT_COLORFILL,&fx)==S_OK);}
void draw(reign::LegacyGles &renderer,NativeEngine::SurfaceTexture &texture,unsigned r,unsigned g,unsigned b){
 renderer.clear(W,H,0,0,0);reign::DrawState state;state.texture_op=reign::TextureOp::texture;state.texture=texture.Refresh(renderer);
 auto vertices=quad(0xffffffff);renderer.draw(W,H,vertices.data(),vertices.size(),indices,6,state);
 std::vector<std::uint8_t> pixels(W*H*4);renderer.read_rgba(W,H,pixels.data());
 auto at=(H/2*W+W/2)*4;check(pixels[at]==r&&pixels[at+1]==g&&pixels[at+2]==b);
}
int main(){try{
 DDSURFACEDESC2 desc{};desc.dwSize=sizeof(desc);desc.dwFlags=DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT|DDSD_CAPS;desc.dwWidth=2;desc.dwHeight=2;desc.ddsCaps.dwCaps=DDSCAPS_SYSTEMMEMORY|DDSCAPS_TEXTURE;
 auto &format=desc.ddpfPixelFormat;format.dwSize=sizeof(format);format.dwFlags=DDPF_RGB;format.dwRGBBitCount=16;format.dwRBitMask=0xf800;format.dwGBitMask=0x7e0;format.dwBBitMask=0x1f;
 IDirectDrawSurface7 *surface=nullptr;check(NativeEngine::CreateMemorySurface(desc,&surface)==S_OK);
 NativeEngine::SurfaceTexture texture(surface);surface->Release(); // texture retains it
 for(int pass=0;pass<2;++pass){
  Context context;reign::LegacyGles renderer(resolve);
  struct Detach {NativeEngine::SurfaceTexture &texture;~Detach(){texture.AbandonContext();}} detach{texture};
  // Exercise actual DX7 virtual draw dispatch, not the fixture device.
  DDSURFACEDESC2 targetDesc=desc;targetDesc.dwWidth=W;targetDesc.dwHeight=H;
  IDirectDrawSurface7 *target=nullptr;check(NativeEngine::CreateMemorySurface(targetDesc,&target)==S_OK);
  IDirect3DDevice7 *device=nullptr;check(NativeEngine::CreateGlesDevice(renderer,target,&device)==S_OK);target->Release();
  auto vertices=quad(0xffff0000);WORD triangleIndices[]={0,1,2,0,2,3};
  check(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,D3DFVF_TLVERTEX,vertices.data(),4,triangleIndices,6,0)==E_INVALIDARG);
  check(device->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1,0)==S_OK);
  check(device->Clear(0,nullptr,D3DCLEAR_TARGET,0xff000000,1,0)==E_NOTIMPL);
  check(device->BeginScene()==S_OK&&device->BeginScene()==E_FAIL);
  check(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,D3DFVF_TLVERTEX,vertices.data(),4,triangleIndices,6,0)==S_OK);
  std::vector<std::uint8_t> result(W*H*4);renderer.read_rgba(W,H,result.data());
  const auto center=(H/2*W+W/2)*4;check(result[center]==255&&result[center+1]==0&&result[center+2]==0);
  triangleIndices[5]=4;check(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,D3DFVF_TLVERTEX,vertices.data(),4,triangleIndices,6,0)==E_INVALIDARG);triangleIndices[5]=3;
  check(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,D3DFVF_XYZ,vertices.data(),4,triangleIndices,6,0)==E_NOTIMPL);
  check(device->SetTexture(1,surface)==E_NOTIMPL);check(device->SetTexture(0,surface)==S_OK);
  check(device->SetTextureStageState(0,D3DTSS_MIPFILTER,D3DTFP_LINEAR)==E_NOTIMPL);
  check(device->SetTextureStageState(0,D3DTSS_MINFILTER,D3DTFN_LINEAR)==S_OK);
  check(device->SetTextureStageState(0,D3DTSS_MAGFILTER,D3DTFG_LINEAR)==S_OK);
  check(device->SetTextureStageState(0,D3DTSS_ADDRESS,D3DTADDRESS_WRAP)==S_OK);
  check(device->SetRenderState(D3DRENDERSTATE_DESTBLEND,D3DBLEND_SRCALPHASAT)==E_NOTIMPL);
  fill(surface,0x07e0);vertices=quad(0xffffffff);
  check(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,D3DFVF_TLVERTEX,vertices.data(),4,triangleIndices,6,0)==S_OK);
  renderer.read_rgba(W,H,result.data());check(result[center]==0&&result[center+1]==255&&result[center+2]==0);
  fill(surface,0x001f);
  check(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,D3DFVF_TLVERTEX,vertices.data(),4,triangleIndices,6,0)==S_OK);
  renderer.read_rgba(W,H,result.data());check(result[center]==0&&result[center+1]==0&&result[center+2]==255);
  check(device->EndScene()==S_OK&&device->EndScene()==E_FAIL);check(device->Release()==0);
  fill(surface,0xf800);draw(renderer,texture,255,0,0);
  const auto first=texture.Refresh(renderer);check(texture.Refresh(renderer)==first);
  fill(surface,0x001f);draw(renderer,texture,0,0,255);
  DDSURFACEDESC2 mapped{};mapped.dwSize=sizeof(mapped);check(surface->Lock(nullptr,&mapped,0,nullptr)==S_OK);
  auto *p=static_cast<std::uint16_t *>(mapped.lpSurface);for(unsigned i=0;i<4;++i)p[i]=0x07e0;
  check(surface->Unlock(nullptr)==S_OK);draw(renderer,texture,0,255,0);
  texture.AbandonContext();renderer.abandon_context();
 }
 std::cout<<"Native DX7 surface -> RGB565 conversion -> GLES texture -> rendered pixels and real DX7 device draw dispatch passed across two contexts; no mission executed\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
