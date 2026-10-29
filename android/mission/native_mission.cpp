#include "mission_app.h"
#include "input_native.h"
#include "audio_android.h"
#include "reign/legacy_gles.hpp"
#include <jni.h>
#include <EGL/egl.h>
#include <android/log.h>
#include <memory>
#include <string>
#include <chrono>
#include <cstdio>
namespace Sound {namespace Digital {void Claim(unsigned,unsigned);}}
namespace {
std::string lastError;
void *resolve(const char *name){return reinterpret_cast<void *>(eglGetProcAddress(name));}
std::string utf(JNIEnv *env,jstring value){if(!value)throw std::invalid_argument("Missing installation path");auto *raw=env->GetStringUTFChars(value,nullptr);if(!raw)throw std::runtime_error("Cannot read path");std::string result(raw);env->ReleaseStringUTFChars(value,raw);return result;}
struct State {
 unsigned width=640,height=480;
 std::unique_ptr<reign::LegacyGles> renderer;
 std::unique_ptr<NativeEngine::MissionApp> app;
 std::unique_ptr<NativeEngine::AndroidAudioOutput> audio;
 bool presented=false;
 unsigned frames=0;
 double frameMillis=0,presentMillis=0;
 reign::LegacyGles::Statistics previousStats;
 std::chrono::steady_clock::time_point sampleTime=std::chrono::steady_clock::now();
 void Present(){auto start=std::chrono::steady_clock::now();renderer->present_to(width,height);presentMillis+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();presented=true;}
 static bool present(void *value){static_cast<State *>(value)->Present();return true;}
 State(const std::string &root,const std::string &work){
  renderer=std::make_unique<reign::LegacyGles>(resolve);
  app=std::make_unique<NativeEngine::MissionApp>(root,work,"2crazed",*renderer,present,this);
  audio=std::make_unique<NativeEngine::AndroidAudioOutput>(NativeEngine::GetDigitalMixer());
  Sound::Digital::Claim(32,32);
  if(!audio->Start())__android_log_print(ANDROID_LOG_WARN,"ReignMission","Audio output unavailable; running muted");
 }
 ~State(){if(audio)audio->Stop();app.reset();audio.reset();renderer.reset();}
};
State &state(jlong handle){if(!handle)throw std::runtime_error("Mission is not initialized");return *reinterpret_cast<State *>(handle);}
jstring failed(JNIEnv *env,const std::exception &e){lastError=e.what();__android_log_print(ANDROID_LOG_ERROR,"ReignMission","%s",lastError.c_str());return env->NewStringUTF(lastError.c_str());}
}
extern "C" {
JNIEXPORT jlong JNICALL Java_org_reigncarnated_menu_MissionActivity_openMission(JNIEnv *env,jclass,jstring root,jstring work){try{lastError.clear();return reinterpret_cast<jlong>(new State(utf(env,root),utf(env,work)));}catch(const std::exception &e){failed(env,e);return 0;}}
JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MissionActivity_missionError(JNIEnv *env,jclass){return env->NewStringUTF(lastError.c_str());}
JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MissionActivity_frameMission(JNIEnv *env,jclass,jlong handle,jint width,jint height){try{auto &s=state(handle);if(width<=0||height<=0)return nullptr;s.width=unsigned(width);s.height=unsigned(height);s.presented=false;auto start=std::chrono::steady_clock::now();s.app->Frame();if(!s.presented)s.Present();s.frameMillis+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();++s.frames;return nullptr;}catch(const std::exception &e){return failed(env,e);}}
JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MissionActivity_performanceMission(JNIEnv *env,jclass,jlong handle){try{
 auto &s=state(handle);const auto now=std::chrono::steady_clock::now();double seconds=std::chrono::duration<double>(now-s.sampleTime).count();const auto stats=s.renderer->statistics();char report[256];const double count=s.frames?s.frames:1;
 std::snprintf(report,sizeof(report),"%.0f draws/frame | %.0f uploads/s (%.2f MiB/s)\nEngine %.1f ms | present submit %.1f ms",double(stats.draws-s.previousStats.draws)/count,double(stats.texture_uploads-s.previousStats.texture_uploads)/seconds,double(stats.texture_bytes-s.previousStats.texture_bytes)/seconds/1048576.0,(s.frameMillis-s.presentMillis)/count,s.presentMillis/count);
 s.previousStats=stats;s.sampleTime=now;s.frames=0;s.frameMillis=s.presentMillis=0;return env->NewStringUTF(report);
 }catch(const std::exception &e){return failed(env,e);}}
JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MissionActivity_activeMission(JNIEnv *env,jclass,jlong handle,jboolean active){try{auto &s=state(handle);if(!active&&s.audio)s.audio->Stop();s.app->SetActive(active);if(active&&s.audio&&!s.audio->Start())__android_log_print(ANDROID_LOG_WARN,"ReignMission","Audio resume unavailable");return nullptr;}catch(const std::exception &e){return failed(env,e);}}
JNIEXPORT void JNICALL Java_org_reigncarnated_menu_MissionActivity_pointerMission(JNIEnv *,jclass,jint x,jint y,jint button,jboolean pressed){NativeEngine::QueuePointer(x,y,button,pressed?TRUE:FALSE);}
JNIEXPORT void JNICALL Java_org_reigncarnated_menu_MissionActivity_keyMission(JNIEnv *,jclass,jint key,jboolean down){if(key>=0&&key<256)NativeEngine::QueueKey(U8(key),down?TRUE:FALSE);}
JNIEXPORT void JNICALL Java_org_reigncarnated_menu_MissionActivity_closeMission(JNIEnv *env,jclass,jlong handle){if(!handle)return;try{delete reinterpret_cast<State *>(handle);}catch(const std::exception &e){failed(env,e);}}
}
