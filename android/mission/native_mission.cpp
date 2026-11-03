#include "mission_app.h"
#include "input_native.h"
#include "audio_android.h"
#include "reign/legacy_gles.hpp"
#include <jni.h>
#include <EGL/egl.h>
#include <android/log.h>
#include <memory>
#include <vector>
#include <sstream>
#include "reign/renderer_checks.hpp"
#include "reign/native_diagnostics.hpp"
#include "reign/frame_profile.hpp"
#include <string>
#include <chrono>
#include <cstdio>
namespace NativeEngine {void SetRenderLayers(unsigned);unsigned RenderLayers();}
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
  NativeEngine::SetRenderLayers(7);renderer=std::make_unique<reign::LegacyGles>(resolve);
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
JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MissionActivity_frameMission(JNIEnv *env,jclass,jlong handle,jint width,jint height){try{auto &s=state(handle);if(width<=0||height<=0)return nullptr;s.width=unsigned(width);s.height=unsigned(height);s.presented=false;auto start=std::chrono::steady_clock::now();{reign::FrameSpan span("Engine");s.app->Frame();}if(!s.presented)s.Present();s.frameMillis+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();++s.frames;return nullptr;}catch(const std::exception &e){return failed(env,e);}}
JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MissionActivity_performanceMission(JNIEnv *env,jclass,jlong handle){try{
 auto &s=state(handle);const auto now=std::chrono::steady_clock::now();double seconds=std::chrono::duration<double>(now-s.sampleTime).count();const auto stats=s.renderer->statistics();char report[512];const double count=s.frames?s.frames:1;
 std::snprintf(report,sizeof(report),"%.0f draws/frame | %.0f uploads/s (%.2f MiB/s)\nEngine %.1f ms | present submit %.1f ms",double(stats.draws-s.previousStats.draws)/count,double(stats.texture_uploads-s.previousStats.texture_uploads)/seconds,double(stats.texture_bytes-s.previousStats.texture_bytes)/seconds/1048576.0,(s.frameMillis-s.presentMillis)/count,s.presentMillis/count);
 std::string details=std::string(report)+"\nGeometry upload "+std::to_string((stats.geometry_ms-s.previousStats.geometry_ms)/count)+" ms | Draw submit "+std::to_string((stats.draw_ms-s.previousStats.draw_ms)/count)+" ms\n"+std::to_string(double(stats.geometry_bytes-s.previousStats.geometry_bytes)/count/1048576)+" MiB geometry/frame | "+std::to_string(double(stats.stream_renewals-s.previousStats.stream_renewals)/count)+" stream renewals/frame\n"+reign::frame_profile();
 s.previousStats=stats;s.sampleTime=now;s.frames=0;s.frameMillis=s.presentMillis=0;return env->NewStringUTF(details.c_str());
 }catch(const std::exception &e){return failed(env,e);}}
JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MissionActivity_activeMission(JNIEnv *env,jclass,jlong handle,jboolean active){try{auto &s=state(handle);if(!active&&s.audio)s.audio->Stop();s.app->SetActive(active);if(active&&s.audio&&!s.audio->Start())__android_log_print(ANDROID_LOG_WARN,"ReignMission","Audio resume unavailable");return nullptr;}catch(const std::exception &e){return failed(env,e);}}
JNIEXPORT void JNICALL Java_org_reigncarnated_menu_MissionActivity_pointerMission(JNIEnv *,jclass,jint x,jint y,jint button,jboolean pressed){NativeEngine::QueuePointer(x,y,button,pressed?TRUE:FALSE);}
JNIEXPORT void JNICALL Java_org_reigncarnated_menu_MissionActivity_keyMission(JNIEnv *,jclass,jint key,jboolean down){if(key>=0&&key<256)NativeEngine::QueueKey(U8(key),down?TRUE:FALSE);}
JNIEXPORT void JNICALL Java_org_reigncarnated_menu_MissionActivity_closeMission(JNIEnv *env,jclass,jlong handle){if(!handle)return;try{delete reinterpret_cast<State *>(handle);}catch(const std::exception &e){failed(env,e);}}
}

extern "C" JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MissionActivity_debugMission(JNIEnv *env,jobject,jlong handle,jint action){try{
 auto &s=state(handle);std::string report;
 if(action==1){const bool mode=s.renderer->synchronized_stream();try{report=reign::renderer_checks(*s.renderer);}catch(...){s.renderer->synchronized_stream(mode);s.renderer->clear(640,480,0,0,0);throw;}s.renderer->clear(640,480,0,0,0);}
 else if(action==2||action==3){s.renderer->synchronized_stream(action==3);report="Stream mode changed";}
 else if(action>=8&&action<=10){NativeEngine::SetRenderLayers(NativeEngine::RenderLayers()^(1u<<(action-8)));report="Render layer mask="+std::to_string(NativeEngine::RenderLayers());}
 else if(action==4){s.renderer->arm_audit();report="Next frame audit armed";}
 else throw std::invalid_argument("Unknown renderer debug action");
 report+="\n"+s.renderer->diagnostics();return env->NewStringUTF(report.c_str());
 }catch(const std::exception &e){return env->NewStringUTF(e.what());}}
extern "C" JNIEXPORT jintArray JNICALL Java_org_reigncarnated_menu_MissionActivity_captureMission(JNIEnv *env,jobject,jlong handle){try{
 auto &s=state(handle);std::vector<std::uint8_t> rgba(640*480*4);s.renderer->read_rgba(640,480,rgba.data());std::vector<jint> argb(640*480);
 for(unsigned i=0;i<argb.size();++i)argb[i]=jint((std::uint32_t(rgba[((479-i/640)*640+i%640)*4+3])<<24)|(std::uint32_t(rgba[((479-i/640)*640+i%640)*4])<<16)|(std::uint32_t(rgba[((479-i/640)*640+i%640)*4+1])<<8)|rgba[((479-i/640)*640+i%640)*4+2]);
 auto result=env->NewIntArray(argb.size());if(result)env->SetIntArrayRegion(result,0,argb.size(),argb.data());return result;
 }catch(const std::exception &e){reign::nativeDiagnostics.record(e.what());return nullptr;}}
extern "C" JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MissionActivity_diagnosticsMission(JNIEnv *env,jobject,jlong handle){try{auto &s=state(handle);return env->NewStringUTF((s.app->Diagnostics()+" display layers="+std::to_string(NativeEngine::RenderLayers())+"\n"+s.renderer->diagnostics()).c_str());}catch(const std::exception &e){return env->NewStringUTF(e.what());}}
