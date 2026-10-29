#include "reign/god_mesh.hpp"
#include "reign/god_pose.hpp"
#include "reign/world_projection.hpp"
#include "reign/legacy_gles.hpp"
#include "reign/resources.hpp"
#include "reign/bitmap_manager.hpp"
#include "reign/engine_file.hpp"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
namespace {
constexpr unsigned W=640,H=480;
void *resolve(const char *name){return reinterpret_cast<void *>(eglGetProcAddress(name));}
struct Context {
 EGLDisplay display=EGL_NO_DISPLAY;EGLSurface surface=EGL_NO_SURFACE;EGLContext context=EGL_NO_CONTEXT;
 Context(){auto platform=reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));if(!platform)throw std::runtime_error("No surfaceless EGL entry point");display=platform(EGL_PLATFORM_SURFACELESS_MESA,nullptr,nullptr);
 if(display==EGL_NO_DISPLAY||!eglInitialize(display,nullptr,nullptr)||!eglBindAPI(EGL_OPENGL_ES_API))throw std::runtime_error("EGL initialization failed");
 EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT_KHR,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,16,EGL_NONE};EGLConfig config;EGLint count=0;
 if(!eglChooseConfig(display,attrs,&config,1,&count)||count!=1)throw std::runtime_error("No GLES3 configuration");
 EGLint size[]={EGL_WIDTH,W,EGL_HEIGHT,H,EGL_NONE};surface=eglCreatePbufferSurface(display,config,size);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};context=eglCreateContext(display,config,EGL_NO_CONTEXT,ca);
 if(surface==EGL_NO_SURFACE||context==EGL_NO_CONTEXT||!eglMakeCurrent(display,surface,surface,context))throw std::runtime_error("GLES3 context creation failed");}
 ~Context(){eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);if(context!=EGL_NO_CONTEXT)eglDestroyContext(display,context);if(surface!=EGL_NO_SURFACE)eglDestroySurface(display,surface);if(display!=EGL_NO_DISPLAY)eglTerminate(display);}
};
}
int main(int argc,char **argv){try{
 if(argc!=4&&argc!=6)throw std::invalid_argument("Usage: god_mesh_gpu_probe installation-root model.god output.ppm [cycle frame]");
 reign::ResourceStore sources(argv[1]);sources.add_stream("gamedata","world");
 if(!sources.add_archive("gamedata","packs/base.zwp")&&!sources.add_archive("gamedata","base.zwp"))throw std::runtime_error("Missing base archive");
 sources.add_archive("world","worlds/sprawl.zwp");
 auto loader=[&](const std::string &name){return sources.load("gamedata",name);};
 auto model=reign::read_god_mesh(loader,argv[2]);auto &mesh=model.geometry;
 auto vertices=argc==6?reign::GodPoseEvaluator(model).sample(argv[4],std::stof(argv[5])).vertices:mesh.vertices;
 if(mesh.vertices.empty()||mesh.faces.empty())throw std::runtime_error("Model has no root geometry");
 auto lo=mesh.vertices.front(),hi=lo;for(auto &p:mesh.vertices)for(unsigned i=0;i<3;++i){lo[i]=std::min(lo[i],p[i]);hi[i]=std::max(hi[i],p[i]);}
 std::array<float,3> center{};float radius=0;for(unsigned i=0;i<3;++i){center[i]=(lo[i]+hi[i])*.5f;radius=std::max(radius,(hi[i]-lo[i])*.5f);}if(radius<=0)throw std::runtime_error("Degenerate model bounds");
 std::array<float,3> position{center[0]+radius*2,center[1]+radius*1.5f,center[2]+radius*3};
 reign::WorldProjection projection(position,{-2,-1.5f,-3},1.0471975512f,float(H)/W,radius*.01f,radius*20);
 std::vector<std::uint8_t> reference;
 for(unsigned generation=0;generation<2;++generation){
  Context context;reign::LegacyGles renderer(resolve);
  reign::BitmapManager bitmaps(loader,[&](const std::string &name){try{loader(name);return true;}catch(const FileSys::FileNotFound&){return false;}},
   {[&](const reign::RgbaImage &image){return renderer.texture_rgba(image.width,image.height,image.pixels.data());},[&](unsigned texture){renderer.delete_texture(texture);},[]{}});
  struct Batch{std::vector<reign::LegacyClip> vertices;std::vector<std::uint16_t> indices;};std::vector<Batch> batches(mesh.buckets.size());
  for(auto &face:mesh.faces){auto &b=batches.at(face.bucket);for(unsigned i=0;i<3;++i){auto p=projection.transform(vertices.at(face.vertices[i]));auto uv=mesh.uvs.at(face.uvs[i]);
   if(b.vertices.size()>=65536)throw std::runtime_error("Probe batch exceeds 16-bit indices");b.indices.push_back(std::uint16_t(b.vertices.size()));b.vertices.push_back({p[0],p[1],p[2],p[3],0xffffffff,0xff000000,uv[0],uv[1]});}}
  renderer.clear(W,H,0,0,0);reign::DrawState state;state.depth_test=true;state.depth_write=true;state.filtered=true;
  for(unsigned i=0;i<batches.size();++i){auto &b=batches[i];if(b.indices.empty())continue;auto &texture=mesh.buckets[i].texture0;
   state.texture=texture&&!texture->name.empty()?bitmaps.load(texture->name)->texture:0;state.texture_op=state.texture?reign::TextureOp::texture:reign::TextureOp::diffuse;
   renderer.draw_clip(W,H,b.vertices.data(),b.vertices.size(),b.indices.data(),b.indices.size(),state);
  }
  renderer.present();std::vector<std::uint8_t> pixels(W*H*4);renderer.read_presented_rgba(W,H,pixels.data());unsigned nonblack=0;for(unsigned i=0;i<W*H;++i)if(pixels[i*4]||pixels[i*4+1]||pixels[i*4+2])++nonblack;
  if(nonblack<100)throw std::runtime_error("Model frame unexpectedly empty");
  if(generation==0){reference=pixels;std::ofstream out(argv[3],std::ios::binary);out<<"P6\n"<<W<<' '<<H<<"\n255\n";for(unsigned row=0;row<H;++row)for(unsigned x=0;x<W;++x)out.write(reinterpret_cast<const char*>(&pixels[((H-1-row)*W+x)*4]),3);if(!out)throw std::runtime_error("Snapshot write failed");}
  else if(reference!=pixels)throw std::runtime_error("Model pixels changed after context recreation");
  std::cout<<"Context "<<generation+1<<" model="<<argv[2]<<" triangles="<<mesh.faces.size()<<" nonblack="<<nonblack<<'\n';
 }
 std::cout<<(argc==6?"PASS: original sampled animation/hierarchy/skinning through GLES. ":"PASS: serialized root geometry through GLES. ")<<"Diagnostic only: no animation clock/transitions, lighting, overlays or original material policy.\n";
}catch(const std::exception &e){std::cerr<<"GOD_GPU_FAIL "<<e.what()<<'\n';return 1;}}
