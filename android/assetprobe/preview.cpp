#include "reign/archive.hpp"
#include "reign/tga.hpp"
#include "reign/legacy_gles.hpp"
#include "reign/renderer_checks.hpp"
#include <EGL/egl.h>
#include <jni.h>
#include <array>
#include <algorithm>
#include <cstring>
#include <string>
namespace {
thread_local std::string last_error;
void *resolve(const char *name){return reinterpret_cast<void *>(eglGetProcAddress(name));}
struct Preview {
 EGLContext context=eglGetCurrentContext();
 reign::LegacyGles renderer{resolve};
 unsigned texture=0,image_w=1,image_h=1;
 std::string report;
 Preview(){report=reign::renderer_checks(renderer);}
};
Preview *get(jlong h){return reinterpret_cast<Preview *>(static_cast<std::uintptr_t>(h));}
}
extern "C" JNIEXPORT jlong JNICALL Java_org_reigncarnated_assetprobe_ProbeActivity_createPreviewNative(JNIEnv *,jobject){
 try{last_error.clear();return static_cast<jlong>(reinterpret_cast<std::uintptr_t>(new Preview));}
 catch(const std::exception &e){last_error=std::string("RENDERER FAIL\n")+e.what();return 0;}
}
extern "C" JNIEXPORT jstring JNICALL Java_org_reigncarnated_assetprobe_ProbeActivity_previewReportNative(JNIEnv *env,jobject,jlong h){
 return env->NewStringUTF(h?get(h)->report.c_str():last_error.c_str());
}
extern "C" JNIEXPORT void JNICALL Java_org_reigncarnated_assetprobe_ProbeActivity_destroyPreviewNative(JNIEnv *,jobject,jlong h,jboolean context_lost){
 if(!h)return;auto *p=get(h);
 if(context_lost||p->context!=eglGetCurrentContext())p->renderer.abandon_context();
 else if(p->texture)p->renderer.delete_texture(p->texture);
 delete p;
}
extern "C" JNIEXPORT jstring JNICALL Java_org_reigncarnated_assetprobe_ProbeActivity_drawPreviewNative(JNIEnv *env,jobject,jlong h,jint w,jint height){
 if(!h||w<32||height<32)return env->NewStringUTF("");
 auto &p=*get(h);
 try{
  p.renderer.clear(w,height,.035f,.045f,.065f);
  float width=std::min(float(w)-24,float(height)-24),image_h=width*p.image_h/p.image_w;
  if(image_h>height-24){image_h=height-24;width=image_h*p.image_w/p.image_h;}
  float x=(w-width)*.5f-.5f,y=(height-image_h)*.5f-.5f;
  std::array<reign::LegacyTL,4> vertices={{{x,y,.5f,1,0xffffffff,0xff000000,0,0},{x+width,y,.5f,1,0xffffffff,0xff000000,1,0},{x+width,y+image_h,.5f,1,0xffffffff,0xff000000,1,1},{x,y+image_h,.5f,1,0xffffffff,0xff000000,0,1}}};
  if(!p.texture){vertices[0].diffuse=0xff2360b0;vertices[1].diffuse=0xff20b898;vertices[2].diffuse=0xffb8e8ff;vertices[3].diffuse=0xff274080;}
  const std::uint16_t indices[]={0,1,2,0,2,3};reign::DrawState state;
  state.texture=p.texture;state.texture_op=p.texture?reign::TextureOp::modulate_rgba:reign::TextureOp::diffuse;
  state.blend=reign::Blend::alpha;p.renderer.draw(w,height,vertices.data(),vertices.size(),indices,6,state);p.renderer.present();
  return env->NewStringUTF("");
 }catch(const std::exception &e){return env->NewStringUTF(e.what());}
}
extern "C" JNIEXPORT jbyteArray JNICALL Java_org_reigncarnated_assetprobe_ProbeActivity_loadAtlasNative(JNIEnv *env,jobject,jstring path){
 const char *raw=env->GetStringUTFChars(path,nullptr);if(!raw)return nullptr;
 std::string file(raw);env->ReleaseStringUTFChars(path,raw);
 try{
  reign::Archive pack(file);auto image=reign::decode_tga(pack.read("cursors.tga"));
  std::vector<std::uint8_t> out(8+image.pixels.size());
  for(unsigned n=0;n<4;++n){out[n]=static_cast<std::uint8_t>(image.width>>(8*n));out[4+n]=static_cast<std::uint8_t>(image.height>>(8*n));}
  std::memcpy(out.data()+8,image.pixels.data(),image.pixels.size());
  auto result=env->NewByteArray(out.size());if(result)env->SetByteArrayRegion(result,0,out.size(),reinterpret_cast<const jbyte *>(out.data()));return result;
 }catch(const std::exception &){return nullptr;}
}
extern "C" JNIEXPORT jstring JNICALL Java_org_reigncarnated_assetprobe_ProbeActivity_uploadAtlasNative(JNIEnv *env,jobject,jlong h,jbyteArray data){
 try{
  if(!h||!data)throw std::runtime_error("No renderer/image");
  auto size=env->GetArrayLength(data);if(size<8)throw std::runtime_error("Truncated preview image");
  std::vector<std::uint8_t> bytes(size);env->GetByteArrayRegion(data,0,size,reinterpret_cast<jbyte *>(bytes.data()));
  auto word=[&](unsigned p){return std::uint32_t(bytes[p])|(std::uint32_t(bytes[p+1])<<8)|(std::uint32_t(bytes[p+2])<<16)|(std::uint32_t(bytes[p+3])<<24);};
  auto w=word(0),height=word(4);if(!w||!height||w>4096||height>4096||std::uint64_t(w)*height*4+8!=bytes.size())throw std::runtime_error("Invalid preview image size");
  auto &p=*get(h);auto texture=p.renderer.texture_rgba(w,height,bytes.data()+8);
  if(p.texture)p.renderer.delete_texture(p.texture);p.texture=texture;p.image_w=w;p.image_h=height;
  return env->NewStringUTF("");
 }catch(const std::exception &e){return env->NewStringUTF(e.what());}
}
