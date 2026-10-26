#include "reign/menu_frontend.hpp"
#include "reign/menu_audio.hpp"
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
 std::unique_ptr<reign::Archive> shell;
 std::unique_ptr<reign::MenuFrontend> menu;
 std::unique_ptr<reign::LegacyGles> renderer;
 std::unique_ptr<reign::FrontendPresenter> presenter;
 std::unique_ptr<reign::MenuAudio> audio;
 State(const std::string &pack,const std::string &profiles,const std::string &shellPath):archive(pack){
  if(!shellPath.empty())shell=std::make_unique<reign::Archive>(shellPath);
  auto loader=[this](const std::string &name){auto at=name.find_last_of("/\\");auto key=at==std::string::npos?name:name.substr(at+1);if(shell&&shell->find(key))return shell->read(key);if(!archive.find(key))throw FileSys::FileNotFound(key);return archive.read(key);};
  audio=std::make_unique<reign::MenuAudio>(loader,std::filesystem::path(profiles).parent_path()/"menu-audio");
  menu=std::make_unique<reign::MenuFrontend>(loader,profiles,640,480,[this](const std::string &name){return audio->load(name);});reset(false);
 }
 void reset(bool lost){if(lost){if(presenter)presenter->abandon_context();if(renderer)renderer->abandon_context();}presenter.reset();renderer.reset();renderer=std::make_unique<reign::LegacyGles>(resolve);presenter=std::make_unique<reign::FrontendPresenter>(*renderer);}
 ~State(){presenter.reset();renderer.reset();}
};
State &state(jlong h){if(!h)throw std::runtime_error("Native menu not initialized");return *reinterpret_cast<State *>(h);}
void failure(const std::exception &e){error=e.what();__android_log_print(ANDROID_LOG_ERROR,"ReignMenu","%s",error.c_str());}
}
extern "C" {
JNIEXPORT jlong JNICALL Java_org_reigncarnated_menu_MenuActivity_openNative(JNIEnv *env,jobject,jstring pack,jstring profiles,jstring shell){try{error.clear();return reinterpret_cast<jlong>(new State(utf(env,pack),utf(env,profiles),shell?utf(env,shell):std::string{}));}catch(const std::exception &e){failure(e);return 0;}}
JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MenuActivity_errorNative(JNIEnv *env,jobject){return env->NewStringUTF(error.c_str());}
JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MenuActivity_drawNative(JNIEnv *env,jobject,jlong h,jint width,jint height,jlong ms){try{auto &s=state(h);s.presenter->begin(640,480);auto report=s.menu->draw([&](const auto &b){s.presenter->submit(b);},[&](const auto &f,const auto &b){s.presenter->submit(f,b);},static_cast<unsigned>(ms),16);s.presenter->present(width,height);return nullptr;}catch(const std::exception &e){failure(e);if(h && state(h).presenter){try{state(h).presenter->cancel();}catch(...){}}return env->NewStringUTF(error.c_str());}}
JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MenuActivity_inputNative(JNIEnv *env,jobject,jlong h,jint kind,jint x,jint y,jstring text){try{auto &s=state(h);if(kind==0)s.menu->pointer_move(x,y);else if(kind==1||kind==2)s.menu->pointer_button(kind==1,x,y);else if(kind==3)s.menu->text(utf(env,text));else if(kind==4){s.menu->key(unsigned(x),true);s.menu->key(unsigned(x),false);}else if(kind==5)s.menu->set_effect_volume(x/100.0f);else throw std::invalid_argument("Unknown input action");auto action=s.menu->take_action();return action.empty()?nullptr:env->NewStringUTF(action.c_str());}catch(const std::exception &e){failure(e);return env->NewStringUTF(error.c_str());}}
JNIEXPORT jfloat JNICALL Java_org_reigncarnated_menu_MenuActivity_volumeNative(JNIEnv *,jobject,jlong h){return h?state(h).menu->effect_volume():1.0f;}
JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MenuActivity_resetGpuNative(JNIEnv *env,jobject,jlong h){try{state(h).reset(true);return nullptr;}catch(const std::exception &e){failure(e);return env->NewStringUTF(error.c_str());}}
JNIEXPORT jobjectArray JNICALL Java_org_reigncarnated_menu_MenuActivity_audioNative(JNIEnv *env,jobject,jlong h){
 try{auto events=state(h).audio->drain();auto cls=env->FindClass("java/lang/String");if(!cls)return nullptr;auto result=env->NewObjectArray(events.size()*2,cls,nullptr);env->DeleteLocalRef(cls);if(!result)return nullptr;
 for(std::size_t i=0;i<events.size();++i){auto path=env->NewStringUTF(events[i].path.c_str());auto volume=env->NewStringUTF(std::to_string(events[i].volume).c_str());if(!path||!volume)return nullptr;env->SetObjectArrayElement(result,i*2,path);env->SetObjectArrayElement(result,i*2+1,volume);env->DeleteLocalRef(path);env->DeleteLocalRef(volume);}return result;
 }catch(const std::exception &e){failure(e);return nullptr;}
}
JNIEXPORT void JNICALL Java_org_reigncarnated_menu_MenuActivity_closeNative(JNIEnv *,jobject,jlong h){if(!h)return;std::unique_ptr<State> s(reinterpret_cast<State *>(h));try{s->menu->close();}catch(const std::exception &e){failure(e);}}
}
