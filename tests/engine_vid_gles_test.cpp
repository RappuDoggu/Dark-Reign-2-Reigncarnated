#include "vid_private.h"
#include "vid_native.h"
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
#include <cstring>
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
}
void check(bool b){if(!b)throw std::runtime_error("Original Vid/GLES integration failed");}
struct Presentation {reign::LegacyGles *renderer;unsigned calls=0;};
bool present(void *context){auto &p=*static_cast<Presentation *>(context);p.renderer->present();++p.calls;return true;}
int main(){try{
 for(unsigned pass=0;pass<2;++pass){
  Vid::Heap::Init(2048,6144);
  struct DrawingBuffers {~DrawingBuffers(){Vid::DoneBuckets();Vid::Heap::Done();}} buffers;
  Vid::InitBuckets();
  check(Vid::Heap::Size()>0&&Vid::Heap::Check());
  Context context;reign::LegacyGles renderer(resolve);Presentation presentation{&renderer};
  check(NativeEngine::AttachGlesVideo(renderer,0,H,present,&presentation)==E_INVALIDARG);
  check(!Vid::device);
  check(NativeEngine::AttachGlesVideo(renderer,W,H,present,&presentation)==S_OK);
  check(NativeEngine::AttachGlesVideo(renderer,W,H,present,&presentation)==E_FAIL);
  struct Detach {~Detach(){NativeEngine::DetachVideo();}} detach;
  check(Bitmap::Manager::Init()&&Material::Manager::Init());
  struct Resources {bool active=true;~Resources(){if(active){Material::Manager::Done();Bitmap::Manager::Done();}}} resources;
  check(Vid::isStatus.initialized);
  check(Vid::device&&Vid::backBmp.GetSurface()&&Vid::viewRect.Width()==W);
  check(Vid::Config::TrilinearOff());
  Vid::InitResources(TRUE);
  check(Material::Manager::GetMaterial()!=nullptr);
  check(Vid::device->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1,0)==S_OK);
  check(Vid::device->SetRenderState(D3DRENDERSTATE_CLIPPING,0)==S_OK);
  check(Vid::device->SetRenderState(D3DRENDERSTATE_CLIPPING,1)==S_OK);
  check(Vid::device->SetRenderState(D3DRENDERSTATE_CLIPPING,2)==E_NOTIMPL);
  check(Vid::device->BeginScene()==S_OK);
  check(!Vid::SetZWriteState(TRUE));check(Vid::SetZWriteState(FALSE));
  reign::LegacyTL vertices[]={{-.5f,-.5f,.5f,1,0xffff0000,0,0,0},{W-.5f,-.5f,.5f,1,0xffff0000,0,1,0},{W-.5f,H-.5f,.5f,1,0xffff0000,0,1,1},{-.5f,H-.5f,.5f,1,0xffff0000,0,0,1}};
  WORD indices[]={0,1,2,0,2,3};
  check(Vid::DrawIndexedPrimitive(PT_TRIANGLELIST,VERTEX_TYPE(D3DFVF_TLVERTEX),vertices,4,indices,6,RS_BLEND_DEF));
  // Reproduce the original engine's odd-address bucket index allocation.
  alignas(WORD) std::array<unsigned char,sizeof(indices)+1> packed{};
  std::memcpy(packed.data()+1,indices,sizeof(indices));
  check(Vid::device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,D3DFVF_TLVERTEX,vertices,4,reinterpret_cast<WORD *>(packed.data()+1),6,0)==S_OK);
  WORD invalid[]={0,1,4};std::memcpy(packed.data()+1,invalid,sizeof(invalid));
  check(Vid::device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,D3DFVF_TLVERTEX,vertices,4,reinterpret_cast<WORD *>(packed.data()+1),3,0)==E_INVALIDARG);
  check(Vid::device->EndScene()==S_OK&&Vid::RenderFlush()&&presentation.calls==1);
  std::vector<std::uint8_t> pixels(W*H*4);renderer.read_presented_rgba(W,H,pixels.data());
  const auto center=(H/2*W+W/2)*4;check(pixels[center]==255&&pixels[center+1]==0&&pixels[center+2]==0);
  Material::Manager::Done();Bitmap::Manager::Done();resources.active=false;
  NativeEngine::DetachVideo();check(!Vid::RenderFlush()&&!Vid::device&&!Vid::backBmp.GetSurface()&&!Vid::isStatus.initialized);
 }
 std::cout<<"Original Vid attachment -> real DX7 device -> GLES -> presented pixels -> detach passed twice; no mission executed\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
