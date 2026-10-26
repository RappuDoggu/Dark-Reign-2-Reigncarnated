#include "reign/menu_frontend.hpp"
#include "reign/frontend_presenter.hpp"
#include "reign/legacy_gles.hpp"
#include "reign/archive.hpp"
#include "reign/engine_file.hpp"
#include <jni.h>
#include <EGL/egl.h>
#include <android/log.h>
#include <memory>
#include <string>
namespace {
thread_local std::string error;
std::string utf(JNIEnv *env,jstring value){if(!value)throw std::invalid_argument("Missing string");const char *raw=env->GetStringUTFChars(value,nullptr);if(!raw)throw std::runtime_error("String allocation failed");std::string s(raw);env->ReleaseStringUTFChars(value,raw);return s;}
void *resolve(const char *name){return reinterpret_cast<void *>(eglGetProcAddress(name));}
struct State {
 reign::Archive archive;
 std::unique_ptr<reign::MenuFrontend> menu;
 std::unique_ptr<reign::LegacyGles> renderer;
 std::unique_ptr<reign::FrontendPresenter> presenter;
 unsigned suppressedSounds=0;
 State(const std::string &pack,const std::string &profiles):archive(pack){
  auto loader=[this](const std::string &name){auto at=name.find_last_of("/\\");auto key=at==std::string::npos?name:name.substr(at+1);if(!archive.find(key))throw FileSys::FileNotFound(key);return archive.read(key);};
  // Explicit disabled-audio backend for this private milestone, no claim of playback.
  auto silent=[this](const std::string &){return std::function<void(float)>([this](float){++suppressedSounds;});};
  menu=std::make_unique<reign::MenuFrontend>(loader,profiles,640,480,silent);reset(false);
 }
 void reset(bool lost){if(lost){if(presenter)presenter->abandon_context();if(renderer)renderer->abandon_context();}presenter.reset();renderer.reset();renderer=std::make_unique<reign::LegacyGles>(resolve);presenter=std::make_unique<reign::FrontendPresenter>(*renderer);}
 ~State(){presenter.reset();renderer.reset();}
};
State &state(jlong h){if(!h)throw std::runtime_error("Native menu not initialized");return *reinterpret_cast<State *>(h);}
void failure(const std::exception &e){error=e.what();__android_log_print(ANDROID_LOG_ERROR,"ReignMenu","%s",error.c_str());}
}
extern "C" {
JNIEXPORT jlong JNICALL Java_org_reigncarnated_menu_MenuActivity_openNative(JNIEnv *env,jobject,jstring pack,jstring profiles){try{error.clear();return reinterpret_cast<jlong>(new State(utf(env,pack),utf(env,profiles)));}catch(const std::exception &e){failure(e);return 0;}}
JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MenuActivity_errorNative(JNIEnv *env,jobject){return env->NewStringUTF(error.c_str());}
JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MenuActivity_drawNative(JNIEnv *env,jobject,jlong h,jint width,jint height,jlong ms){try{auto &s=state(h);s.presenter->begin(640,480);auto report=s.menu->draw([&](const auto &b){s.presenter->submit(b);},[&](const auto &f,const auto &b){s.presenter->submit(f,b);},static_cast<unsigned>(ms),16);s.presenter->present(width,height);return nullptr;}catch(const std::exception &e){failure(e);if(h && state(h).presenter){try{state(h).presenter->cancel();}catch(...){}}return env->NewStringUTF(error.c_str());}}
JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MenuActivity_inputNative(JNIEnv *env,jobject,jlong h,jint kind,jint x,jint y,jstring text){try{auto &s=state(h);if(kind==0)s.menu->pointer_move(x,y);else if(kind==1||kind==2)s.menu->pointer_button(kind==1,x,y);else if(kind==3)s.menu->text(utf(env,text));else if(kind==4){s.menu->key(unsigned(x),true);s.menu->key(unsigned(x),false);}else throw std::invalid_argument("Unknown input action");return nullptr;}catch(const std::exception &e){failure(e);return env->NewStringUTF(error.c_str());}}
JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MenuActivity_resetGpuNative(JNIEnv *env,jobject,jlong h){try{state(h).reset(true);return nullptr;}catch(const std::exception &e){failure(e);return env->NewStringUTF(error.c_str());}}
JNIEXPORT void JNICALL Java_org_reigncarnated_menu_MenuActivity_closeNative(JNIEnv *,jobject,jlong h){if(!h)return;std::unique_ptr<State> s(reinterpret_cast<State *>(h));try{s->menu->close();}catch(const std::exception &e){failure(e);}}
}
