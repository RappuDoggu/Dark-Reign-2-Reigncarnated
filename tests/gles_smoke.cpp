#include "reign/renderer_checks.hpp"
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstring>
#include <EGL/eglext.h>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
#ifdef REIGN_ORIGINAL_VID_CHECKS
void original_vid_gpu_checks(reign::LegacyGles &);
void original_bucket_gpu_checks(reign::LegacyGles &);
void original_bitmap_gpu_checks(reign::LegacyGles &);
void original_bitmap_manager_gpu_checks(reign::LegacyGles &);
void original_font_gpu_checks(reign::LegacyGles &);
void original_frontend_gpu_checks(reign::LegacyGles &);
void original_presenter_gpu_checks(reign::LegacyGles &);
#endif
#ifdef REIGN_WORLD_PROJECTION_CHECKS
void original_world_gpu_checks(reign::LegacyGles &);
#endif
namespace {
constexpr unsigned W=32,H=32;
void *resolve(const char *name){return reinterpret_cast<void *>(eglGetProcAddress(name));}
unsigned probeBuffers=0,probeFinishes=0;
void GL_APIENTRY probeGen(GLsizei n,GLuint *out){probeBuffers+=n;reinterpret_cast<PFNGLGENBUFFERSPROC>(resolve("glGenBuffers"))(n,out);}
GLenum GL_APIENTRY probeWait(GLsync,GLbitfield,GLuint64){return GL_TIMEOUT_EXPIRED;}
void GL_APIENTRY probeFinish(){++probeFinishes;reinterpret_cast<PFNGLFINISHPROC>(resolve("glFinish"))();}
void *probeResolve(const char *name){if(!std::strcmp(name,"glGenBuffers"))return reinterpret_cast<void*>(probeGen);if(!std::strcmp(name,"glClientWaitSync"))return reinterpret_cast<void*>(probeWait);if(!std::strcmp(name,"glFinish"))return reinterpret_cast<void*>(probeFinish);return resolve(name);}
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
int main(){
 try{for(int pass=0;pass<2;++pass){Context context;reign::LegacyGles renderer(resolve);std::cout<<"Context "<<pass+1<<"\n"<<reign::renderer_checks(renderer);
#ifdef REIGN_WORLD_PROJECTION_CHECKS
 original_world_gpu_checks(renderer);
#endif
#ifdef REIGN_ORIGINAL_VID_CHECKS
 original_vid_gpu_checks(renderer);
 original_bucket_gpu_checks(renderer);
 original_bitmap_gpu_checks(renderer);
 original_bitmap_manager_gpu_checks(renderer);
 original_font_gpu_checks(renderer);
 original_frontend_gpu_checks(renderer);original_presenter_gpu_checks(renderer);
#endif
{probeBuffers=probeFinishes=0;reign::LegacyGles pooled(probeResolve);reign::DrawState state;auto red=quad(0xffff0000);
 for(unsigned frame=0;frame<12;++frame){pooled.clear(W,H,0,0,0);pooled.draw(W,H,red.data(),red.size(),indices,6,state);pooled.present();}
 if(probeBuffers!=16||probeFinishes!=1)throw std::runtime_error("Unsignaled bank was reused or buffer pool grew beyond its cap");
 std::cout<<"Forced unsignaled fences: eight physical banks, bounded GPU wait, safe reuse PASS\n";}
}return 0;}
 catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}

