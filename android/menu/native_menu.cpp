#include "reign/menu_frontend.hpp"
#include "reign/menu_audio.hpp"
#include "reign/shell_scene.hpp"
#include "reign/shell_menu_navigation.hpp"
#ifdef REIGN_BINK
#include "reign/bink_movie.hpp"
#endif
#include "reign/frontend_presenter.hpp"
#include "reign/legacy_gles.hpp"
#include "reign/archive.hpp"
#include "reign/mission_menu_catalog.hpp"
#include "reign/config.hpp"
#include <cstdio>
#include "reign/engine_file.hpp"
#include "crc.h"
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
namespace {
thread_local std::string error;
std::string utf(JNIEnv *env,jstring value){if(!value)throw std::invalid_argument("Missing string");const char *raw=env->GetStringUTFChars(value,nullptr);if(!raw)throw std::runtime_error("String allocation failed");std::string s(raw);env->ReleaseStringUTFChars(value,raw);return s;}
void *resolve(const char *name){return reinterpret_cast<void *>(eglGetProcAddress(name));}
struct State {
#ifdef REIGN_BINK
 std::unique_ptr<reign::BinkMovie> movie;unsigned movieIndex=0,movieTexture=0,skipArmedMovie=0;std::int64_t movieFrame=-1;jlong movieStart=-1;
 void nextMovie(){if(movieTexture&&renderer)renderer->delete_texture(movieTexture);movieTexture=0;movie.reset();auto name="intro-movie-"+std::to_string(movieIndex++)+".bik";if(archive.find(name)){movie=std::make_unique<reign::BinkMovie>(archive.read(name));reign::nativeDiagnostics.record("Intro start "+name);}else reign::nativeDiagnostics.record(movieIndex==4?"Intro sequence complete":"Intro clip missing "+name);movieStart=-1;movieFrame=-1;startMs=-1;}
 bool drawMovie(jlong ms,int width,int height){
  if(!movie)return false;if(movieStart<0)movieStart=ms;
  if(!movie->advance(double(ms-movieStart)/1000)){nextMovie();renderer->clear(640,480,0,0,0);renderer->present_to(width,height);return bool(movie);}
  if(movieFrame!=movie->frame_index()){if(movieTexture)renderer->update_texture_rgba(movieTexture,movie->width(),movie->height(),movie->rgba().data());else movieTexture=renderer->texture_rgba(movie->width(),movie->height(),movie->rgba().data());movieFrame=movie->frame_index();}
  renderer->clear(640,480,0,0,0);float scale=std::min(640.f/movie->width(),480.f/movie->height());float w=movie->width()*scale,h=movie->height()*scale,x=(640-w)*.5f,y=(480-h)*.5f;
  reign::LegacyTL v[]={{x,y,0,1,0xffffffff,0xff000000,0,0},{x+w,y,0,1,0xffffffff,0xff000000,1,0},{x+w,y+h,0,1,0xffffffff,0xff000000,1,1},{x,y+h,0,1,0xffffffff,0xff000000,0,1}};std::uint16_t indices[]={0,1,2,0,2,3};reign::DrawState state;state.texture=movieTexture;state.texture_op=reign::TextureOp::texture;state.filtered=true;renderer->draw(640,480,v,4,indices,6,state);renderer->present_to(width,height);return true;
 }
#endif
 reign::LegacyGles::Statistics previousStats;std::chrono::steady_clock::time_point sampleTime=std::chrono::steady_clock::now();
 std::string dataPath,ambientPath;std::filesystem::path progressFile;
 std::unique_ptr<reign::ShellMenuNavigation> navigation;
 unsigned savedMap=0,savedOpponents=0,savedDifficulty=0,savedSeed=1;
 jlong startMs=-1,lastMs=0,frameMs=0,resumeClock=-1;
 float seconds()const{return float(std::max<jlong>(0,frameMs-startMs))/1000.f;}
 void beginInstant(){
#ifdef REIGN_BINK
  if(movie)throw std::runtime_error("Wait for the introduction to finish");
#endif
  if(startMs<0)throw std::runtime_error("Wait for the main menu entrance to finish");
  auto maps=reign::menu_missions(dataPath,false);menu->prepare_instant_setup(maps,difficulties(),savedMap,savedOpponents,savedDifficulty,savedSeed);
  auto path=navigation->begin_instant(seconds(),scene->camera_finished(seconds()));
  try{scene->select_camera(path,seconds());menu->hide_shell_controls();}catch(...){navigation->reset();menu->show_main();throw;}
  reign::nativeDiagnostics.record("Instant Action: "+path+" started; waiting for original control activation event");
 }
 void returnMain(){auto path=navigation->return_main(seconds());if(path.empty())return;scene->select_camera(path,seconds());menu->hide_shell_controls();reign::nativeDiagnostics.record("Instant Action: "+path+" return started");}
 std::vector<std::string> difficulties(){auto cfg=reign::parse_config([this](const std::string &name){return archive.read(name);},"difficulty.cfg");std::vector<std::string> names{""};for(const auto &n:cfg)if(n.name=="CreateSetting"&&!n.arguments.empty())names.push_back(std::get<std::string>(n.arguments[0]));return names;}
 reign::Archive archive;
 std::unique_ptr<reign::Archive> shell;
 std::unique_ptr<reign::MenuFrontend> menu;
 std::unique_ptr<reign::LegacyGles> renderer;
 std::unique_ptr<reign::FrontendPresenter> presenter;
 // Scene destruction uses renderer, including constructor exception unwinding.
 std::unique_ptr<reign::ShellScene> scene;
 std::unique_ptr<reign::MenuAudio> audio;
 State(const std::string &pack,const std::string &profiles,const std::string &shellPath,const std::string &version):dataPath(std::filesystem::path(pack).parent_path().string()),archive(pack){
  char profileId[9];std::snprintf(profileId,sizeof(profileId),"%08X",unsigned(Crc::CalcStr("Commander")));progressFile=std::filesystem::path(profiles).parent_path()/"mission-runtime/users"/profileId/"data.cfg";
  if(!shellPath.empty())shell=std::make_unique<reign::Archive>(shellPath);
  auto loader=[this](const std::string &name){auto at=name.find_last_of("/\\");auto key=at==std::string::npos?name:name.substr(at+1);if(shell&&shell->find(key))return shell->read(key);if(!archive.find(key))throw FileSys::FileNotFound(key);return archive.read(key);};
  navigation=std::make_unique<reign::ShellMenuNavigation>(reign::parse_config(loader,"objectives_cineractives.cfg"));
  audio=std::make_unique<reign::MenuAudio>(loader,std::filesystem::path(profiles).parent_path()/"menu-audio");
  menu=std::make_unique<reign::MenuFrontend>(loader,profiles,640,480,[this](const std::string &name){return audio->load(name);});menu->use_original_instant_setup([this](const reign::MenuMission &map){return reign::menu_mission_loader(dataPath,map);});menu->set_build_version(version);menu->open_shell_preview();reset(false);
#ifdef REIGN_BINK
 nextMovie();
#endif
 }
 void reset(bool lost){
 navigation->reset();if(menu)menu->show_main();previousStats={};sampleTime=std::chrono::steady_clock::now();
#ifdef REIGN_BINK
 if(movieTexture&&renderer&&!lost)renderer->delete_texture(movieTexture);movieTexture=0;movieFrame=-1;
#endif
if(lost){if(scene)scene->abandon_context();if(presenter)presenter->abandon_context();if(renderer)renderer->abandon_context();}scene.reset();presenter.reset();renderer.reset();renderer=std::make_unique<reign::LegacyGles>(resolve);presenter=std::make_unique<reign::FrontendPresenter>(*renderer);scene=std::make_unique<reign::ShellScene>(dataPath,*renderer);auto ambient=scene->ambient_sound();if(!ambient.empty())ambientPath=audio->cached_path(ambient);}
 ~State(){
#ifdef REIGN_BINK
 if(movieTexture&&renderer)renderer->delete_texture(movieTexture);
#endif
 scene.reset();presenter.reset();renderer.reset();}
};
State &state(jlong h){if(!h)throw std::runtime_error("Native menu not initialized");return *reinterpret_cast<State *>(h);}
void failure(const std::exception &e){error=e.what();__android_log_print(ANDROID_LOG_ERROR,"ReignMenu","%s",error.c_str());}
}
extern "C" {
JNIEXPORT jlong JNICALL Java_org_reigncarnated_menu_MenuActivity_openNative(JNIEnv *env,jobject,jstring pack,jstring profiles,jstring shell,jstring version){try{error.clear();return reinterpret_cast<jlong>(new State(utf(env,pack),utf(env,profiles),shell?utf(env,shell):std::string{},utf(env,version)));}catch(const std::exception &e){failure(e);return 0;}}
// Checkpoints contain launch progress only, never GPU handles or filesystem paths.
JNIEXPORT jlongArray JNICALL Java_org_reigncarnated_menu_MenuActivity_checkpointNative(JNIEnv *env,jobject,jlong h){
 auto &s=state(h);jlong values[2]={0,s.startMs<0?0:std::max<jlong>(0,s.frameMs-s.startMs)};
#ifdef REIGN_BINK
 if(s.movie){values[0]=s.movieIndex;values[1]=s.movieStart<0?0:std::max<jlong>(0,s.frameMs-s.movieStart);}
#endif
 auto result=env->NewLongArray(2);if(result)env->SetLongArrayRegion(result,0,2,values);return result;
}
JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MenuActivity_restoreNative(JNIEnv *env,jobject,jlong h,jlong clip,jlong elapsed,jlong now){try{
 if(clip<0||clip>256||elapsed<0||elapsed>now)throw std::runtime_error("Invalid launch checkpoint");auto &s=state(h);
#ifdef REIGN_BINK
 if(s.movieTexture)s.renderer->delete_texture(s.movieTexture);s.movieTexture=0;s.movieFrame=-1;s.movie.reset();
 if(clip){s.movieIndex=unsigned(clip-1);s.nextMovie();if(!s.movie)throw std::runtime_error("Checkpoint intro is missing");
  double target=double(elapsed)/1000;if(target>=s.movie->duration())throw std::runtime_error("Checkpoint exceeds intro duration");
  // Sequential Bink decode rebuilds reference frames. Drain skipped PCM to keep
  // memory bounded and avoid replaying audio preceding the saved position.
  s.movie->restore_position(target);s.movieStart=now-elapsed;s.movieFrame=-1;
 }
#endif
 s.startMs=clip?-1:now-elapsed;s.lastMs=now;s.frameMs=now;s.resumeClock=now;return nullptr;
 }catch(const std::exception &e){failure(e);return env->NewStringUTF(error.c_str());}}
JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MenuActivity_errorNative(JNIEnv *env,jobject){return env->NewStringUTF(error.c_str());}
JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MenuActivity_drawNative(JNIEnv *env,jobject,jlong h,jint width,jint height,jlong ms){try{auto &s=state(h);
 if(s.resumeClock>=0){auto delay=ms-s.resumeClock;if(s.startMs>=0)s.startMs+=delay;
#ifdef REIGN_BINK
 if(s.movieStart>=0)s.movieStart+=delay;
#endif
 s.lastMs=ms;s.resumeClock=-1;}
 s.frameMs=ms;
#ifdef REIGN_BINK
 if(s.drawMovie(ms,width,height))return nullptr;
#endif
 if(s.startMs<0){s.startMs=ms;s.lastMs=ms;}auto step=static_cast<unsigned>(std::min<jlong>(100,std::max<jlong>(0,ms-s.lastMs)));s.lastMs=ms;s.presenter->begin(640,480);{reign::FrameSpan span("Scene");s.scene->draw(float(ms-s.startMs)/1000.f);}for(auto &sound:s.scene->take_sounds())s.audio->load(sound)(1.f);auto event=s.navigation->advance(s.seconds());
 if(event==reign::ShellMenuNavigation::Event::ShowInstant){s.menu->show_instant_setup();reign::nativeDiagnostics.record("GameSetup::Instant activated by original CineractiveCompleted schedule");}
 if(event==reign::ShellMenuNavigation::Event::ShowMain){s.menu->show_main();reign::nativeDiagnostics.record("Shell::Main activated after Instant Action return");}
 if(s.navigation->main_ready(s.scene->camera_finished(s.seconds()))||(s.navigation->page()==reign::ShellMenuNavigation::Page::Instant||s.navigation->page()==reign::ShellMenuNavigation::Page::Launching))s.menu->draw([&](const auto &b){s.presenter->submit(b);},[&](const auto &f,const auto &b){s.presenter->submit(f,b);},static_cast<unsigned>(ms-s.startMs),step);s.scene->finish_frame(float(ms-s.startMs)/1000.f);{reign::FrameSpan span("Present");s.presenter->present(width,height);}return event==reign::ShellMenuNavigation::Event::ShowInstant?env->NewStringUTF("@instant-open"):event==reign::ShellMenuNavigation::Event::ShowMain?env->NewStringUTF("@main-open"):nullptr;}catch(const std::exception &e){failure(e);if(h && state(h).presenter){try{state(h).presenter->cancel();}catch(...){}}return env->NewStringUTF(error.c_str());}}
JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MenuActivity_inputNative(JNIEnv *env,jobject,jlong h,jint kind,jint x,jint y,jstring text){try{auto &s=state(h);
 if(kind==5){s.menu->set_effect_volume(x/100.0f);return nullptr;}
#ifdef REIGN_BINK
 if(s.movie){if(kind==1&&x>=0&&x<640&&y>=0&&y<480)s.skipArmedMovie=s.movieIndex;
 if(kind==4||(kind==2&&s.skipArmedMovie==s.movieIndex&&x>=0&&x<640&&y>=0&&y<480)){reign::nativeDiagnostics.record("Intro explicitly skipped clip "+std::to_string(s.movieIndex));s.nextMovie();}if(kind==2)s.skipArmedMovie=0;return nullptr;}
#endif
 if(s.startMs<0||(!s.navigation->main_ready(s.scene->camera_finished(s.seconds()))&&s.navigation->page()!=reign::ShellMenuNavigation::Page::Instant))return nullptr;
 if(kind==0)s.menu->pointer_move(x,y);else if(kind==1||kind==2)s.menu->pointer_button(kind==1,x,y);else if(kind==3)s.menu->text(utf(env,text));else if(kind==4){s.menu->key(unsigned(x),true);s.menu->key(unsigned(x),false);}else if(kind==5)s.menu->set_effect_volume(x/100.0f);else throw std::invalid_argument("Unknown input action");auto action=s.menu->take_action();if(action=="@instant"){s.beginInstant();action="@instant-started";}
 else if(action=="@instant-back"){s.returnMain();action="@instant-returning";}
 else if(action=="@instant-launch"){
  auto selection=s.menu->instant_selection();s.navigation->launch();s.savedMap=selection.map_index;s.savedOpponents=selection.opponents;s.savedDifficulty=selection.difficulty_index;s.savedSeed=selection.seed;
  std::ostringstream out;out<<"@instant-launch\t"<<selection.map.group<<'\t'<<selection.map.name<<'\t'<<selection.opponents<<'\t'<<selection.seed<<'\t'<<selection.difficulty<<'\t'<<selection.map_index<<'\t'<<selection.difficulty_index;action=out.str();
 }else if(action.empty()&&kind==2&&s.menu->text_input_active())action="@keyboard";
 return action.empty()?nullptr:env->NewStringUTF(action.c_str());}catch(const std::exception &e){failure(e);return env->NewStringUTF(error.c_str());}}
JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MenuActivity_ambientNative(JNIEnv *env,jobject,jlong h){return h&&!state(h).ambientPath.empty()?env->NewStringUTF(state(h).ambientPath.c_str()):nullptr;}
JNIEXPORT jintArray JNICALL Java_org_reigncarnated_menu_MenuActivity_movieInfoNative(JNIEnv *env,jobject,jlong h){jint values[2]={0,0};
#ifdef REIGN_BINK
 if(h&&state(h).movie){values[0]=state(h).movieIndex;values[1]=state(h).movie->sample_rate();}
#endif
 auto result=env->NewIntArray(2);if(result)env->SetIntArrayRegion(result,0,2,values);return result;}
JNIEXPORT jshortArray JNICALL Java_org_reigncarnated_menu_MenuActivity_moviePcmNative(JNIEnv *env,jobject,jlong h){
#ifdef REIGN_BINK
 if(h&&state(h).movie){auto pcm=state(h).movie->drain_audio();auto result=env->NewShortArray(pcm.size());if(result&&!pcm.empty())env->SetShortArrayRegion(result,0,pcm.size(),pcm.data());return result;}
#endif
 return nullptr;}
JNIEXPORT jfloat JNICALL Java_org_reigncarnated_menu_MenuActivity_volumeNative(JNIEnv *,jobject,jlong h){return h?state(h).menu->effect_volume():1.0f;}
JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MenuActivity_resetGpuNative(JNIEnv *env,jobject,jlong h){try{state(h).reset(true);return nullptr;}catch(const std::exception &e){failure(e);return env->NewStringUTF(error.c_str());}}
JNIEXPORT jobjectArray JNICALL Java_org_reigncarnated_menu_MenuActivity_audioNative(JNIEnv *env,jobject,jlong h){
 try{auto events=state(h).audio->drain();auto cls=env->FindClass("java/lang/String");if(!cls)return nullptr;auto result=env->NewObjectArray(events.size()*2,cls,nullptr);env->DeleteLocalRef(cls);if(!result)return nullptr;
 for(std::size_t i=0;i<events.size();++i){auto path=env->NewStringUTF(events[i].path.c_str());auto volume=env->NewStringUTF(std::to_string(events[i].volume).c_str());if(!path||!volume)return nullptr;env->SetObjectArrayElement(result,i*2,path);env->SetObjectArrayElement(result,i*2+1,volume);env->DeleteLocalRef(path);env->DeleteLocalRef(volume);}return result;
 }catch(const std::exception &e){failure(e);return nullptr;}
}
JNIEXPORT void JNICALL Java_org_reigncarnated_menu_MenuActivity_closeNative(JNIEnv *,jobject,jlong h){if(!h)return;std::unique_ptr<State> s(reinterpret_cast<State *>(h));try{s->menu->close();}catch(const std::exception &e){failure(e);}}
}


extern "C" JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MenuActivity_debugNative(JNIEnv *env,jobject,jlong handle,jint action){try{
 auto &s=state(handle);std::string report;
 if(action==1){const bool mode=s.renderer->synchronized_stream();try{report=reign::renderer_checks(*s.renderer);}catch(...){s.renderer->synchronized_stream(mode);s.renderer->clear(640,480,0,0,0);throw;}s.renderer->clear(640,480,0,0,0);}
 else if(action==2||action==3){s.renderer->synchronized_stream(action==3);report="Stream mode changed";}
 else if(action==4){s.renderer->arm_audit();report="Next frame audit armed";}
 else throw std::invalid_argument("Unknown renderer debug action");
 report+="\n"+s.renderer->diagnostics();return env->NewStringUTF(report.c_str());
 }catch(const std::exception &e){return env->NewStringUTF(e.what());}}
extern "C" JNIEXPORT jintArray JNICALL Java_org_reigncarnated_menu_MenuActivity_captureNative(JNIEnv *env,jobject,jlong handle){try{
 auto &s=state(handle);std::vector<std::uint8_t> rgba(640*480*4);s.renderer->read_rgba(640,480,rgba.data());std::vector<jint> argb(640*480);
 for(unsigned i=0;i<argb.size();++i)argb[i]=jint((std::uint32_t(rgba[((479-i/640)*640+i%640)*4+3])<<24)|(std::uint32_t(rgba[((479-i/640)*640+i%640)*4])<<16)|(std::uint32_t(rgba[((479-i/640)*640+i%640)*4+1])<<8)|rgba[((479-i/640)*640+i%640)*4+2]);
 auto result=env->NewIntArray(argb.size());if(result)env->SetIntArrayRegion(result,0,argb.size(),argb.data());return result;
 }catch(const std::exception &e){reign::nativeDiagnostics.record(e.what());return nullptr;}}
extern "C" JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MenuActivity_diagnosticsNative(JNIEnv *env,jobject,jlong handle){try{auto &s=state(handle);return env->NewStringUTF(("Shell camera="+s.scene->camera_name(s.seconds())+" phase="+std::to_string(int(s.navigation->page()))+"\n"+s.renderer->diagnostics()).c_str());}catch(const std::exception &e){return env->NewStringUTF(e.what());}}

extern "C" JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MenuActivity_performanceNative(JNIEnv *env,jobject,jlong handle){try{
 auto &s=state(handle);auto stats=s.renderer->statistics();auto now=std::chrono::steady_clock::now();double seconds=std::max(0.001,std::chrono::duration<double>(now-s.sampleTime).count());double frames=std::max<std::uint64_t>(1,stats.presents-s.previousStats.presents);
 std::ostringstream out;out.precision(3);out<<(stats.draws-s.previousStats.draws)/frames<<" draws/frame | "<<(stats.texture_uploads-s.previousStats.texture_uploads)/seconds<<" uploads/s\nGeometry upload "<<(stats.geometry_ms-s.previousStats.geometry_ms)/frames<<" ms | Draw submit "<<(stats.draw_ms-s.previousStats.draw_ms)/frames<<" ms\n"<<(stats.geometry_bytes-s.previousStats.geometry_bytes)/frames/1048576<<" MiB geometry/frame | "<<(stats.stream_renewals-s.previousStats.stream_renewals)/frames<<" stream renewals/frame\n"<<reign::frame_profile();
 s.previousStats=stats;s.sampleTime=now;return env->NewStringUTF(out.str().c_str());}catch(const std::exception &e){return env->NewStringUTF(e.what());}}

extern "C" JNIEXPORT jobjectArray JNICALL Java_org_reigncarnated_menu_MenuActivity_catalogNative(JNIEnv *env,jobject,jlong handle,jboolean campaign){try{auto rows=reign::menu_missions(state(handle).dataPath,campaign,state(handle).progressFile);auto result=env->NewObjectArray(rows.size(),env->FindClass("java/lang/String"),nullptr);for(unsigned i=0;i<rows.size();++i){auto row=env->NewStringUTF((rows[i].group+"\t"+rows[i].name).c_str());env->SetObjectArrayElement(result,i,row);env->DeleteLocalRef(row);}return result;}catch(const std::exception&e){failure(e);return nullptr;}}
extern "C" JNIEXPORT jobjectArray JNICALL Java_org_reigncarnated_menu_MenuActivity_difficultyNative(JNIEnv *env,jobject,jlong handle){try{auto &s=state(handle);auto cfg=reign::parse_config([&](const std::string&name){return s.archive.read(name);},"difficulty.cfg");std::vector<std::string> names{""};for(const auto&n:cfg)if(n.name=="CreateSetting"&&!n.arguments.empty())names.push_back(std::get<std::string>(n.arguments[0]));auto result=env->NewObjectArray(names.size(),env->FindClass("java/lang/String"),nullptr);for(unsigned i=0;i<names.size();++i){auto name=env->NewStringUTF(names[i].c_str());env->SetObjectArrayElement(result,i,name);env->DeleteLocalRef(name);}return result;}catch(const std::exception&e){failure(e);return nullptr;}}

extern "C" JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MenuActivity_beginInstantNative(JNIEnv *env,jobject,jlong handle){try{state(handle).beginInstant();return nullptr;}catch(const std::exception &e){failure(e);return env->NewStringUTF(e.what());}}
extern "C" JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MenuActivity_returnMainNative(JNIEnv *env,jobject,jlong handle){try{state(handle).returnMain();return nullptr;}catch(const std::exception &e){failure(e);return env->NewStringUTF(e.what());}}

extern "C" JNIEXPORT jstring JNICALL Java_org_reigncarnated_menu_MenuActivity_setInstantOptionsNative(JNIEnv *env,jobject,jlong handle,jint map,jint opponents,jint difficulty,jint seed){try{
 if(map<0||opponents<0||opponents>1||difficulty<0||seed<=0)throw std::invalid_argument("Invalid saved Instant Action options");auto &s=state(handle);s.savedMap=map;s.savedOpponents=opponents;s.savedDifficulty=difficulty;s.savedSeed=seed;return nullptr;
}catch(const std::exception &e){failure(e);return env->NewStringUTF(e.what());}}
