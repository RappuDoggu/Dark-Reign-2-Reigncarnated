#include "vid_private.h"
#include "native_stack.h"
#include <typeinfo>
#include <stdexcept>
extern "C" [[noreturn]] void __real___cxa_throw(void *,void *,void (*)(void *));
extern "C" [[noreturn]] void __wrap___cxa_throw(void *exception,void *type,void (*destroy)(void *)){
 if(*static_cast<std::type_info *>(type)==typeid(std::runtime_error))std::fprintf(stderr,"Runtime error: %s\n",static_cast<std::runtime_error *>(exception)->what());
 std::uintptr_t frames[16];const auto count=NativeEngine::CaptureStack(frames,16);
 for(std::size_t i=0;i<count;++i)std::fprintf(stderr,"Exception caller: %p\n",reinterpret_cast<void *>(frames[i]));
 __real___cxa_throw(exception,type,destroy);
}
#include "iface.h"
#include "event.h"
#include "input.h"
#include "fontsys.h"
#include "multilanguage.h"
#include "console.h"
#include "game.h"
#include "user.h"
#include "sound.h"
#include "gamesound.h"
#include "clock.h"
#include "perfstats.h"
#include "gametime.h"
#include "game_rc.h"
#include "frame_native.h"
#include "orders.h"
#include "sync.h"
#include "demo.h"
#include "missions.h"
#include "campaigns.h"
#include "difficulty.h"
#include "mods.h"
#include "sides.h"
#include "savegame.h"
#include "session_native.h"
#include "multiplayer.h"
#include "multiplayer_controls_report.h"
#include "multiplayer_controls_synclist.h"
#include "multiplayer_controls_playerlist.h"
#include "multiplayer_controls_chatedit.h"
#include "multiplayer_controls_playerinfo.h"
#include "multiplayer_controls_localpings.h"
#include "ptree.h"
#include <filesystem>
#include <chrono>
#include <thread>
#include "light_priv.h"
#include "mesh.h"
#include "camera.h"
#include "terrain_priv.h"
#include "filesys.h"
#include "tracksys.h"
#include "varsys.h"
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
#include <fstream>
namespace Vid {namespace Command {void InitMesh();void DoneMesh();}}
#include "runtime_setup_native.h"
namespace {
constexpr unsigned W=640,H=360;
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
#define check(b) do{if(!(b))throw std::runtime_error("Graphics startup failed at line "+std::to_string(__LINE__));}while(0)
struct Presentation {reign::LegacyGles *renderer;unsigned calls=0;};
bool present(void *context){auto &p=*static_cast<Presentation *>(context);p.renderer->present();++p.calls;return true;}
int main(int argc,char **argv){try{
 const auto previousPath=std::filesystem::current_path();
 const auto profilePath=std::filesystem::temp_directory_path()/("reign-startup-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(profilePath);std::filesystem::current_path(profilePath);
 struct ProfilePath {std::filesystem::path previous,path;~ProfilePath(){std::filesystem::current_path(previous);std::filesystem::remove_all(path);}} profile{previousPath,profilePath};
 if(argc!=2)throw std::runtime_error("Provide owned base.zwp path");
 const auto installation=std::filesystem::path(argv[1]).parent_path().parent_path();
 for(const char *name:{"missions","worlds","sides","packs"})if(std::filesystem::exists(installation/name))std::filesystem::create_directory_symlink(installation/name,profilePath/name);
 Clock::Time::Init();
 TrackSys::Init();VarSys::Init();FileSys::Init();EventSys::Init();MultiLanguage::Init();FontSys::Init();Console::Init();Input::Init(nullptr,nullptr);Sound::Init();GameSound::Init();PerfStats::Init();
 struct Foundation {~Foundation(){VarSys::DeleteItem("vid");VarSys::DeleteItem("gamegod");VarSys::DeleteItem("sys");VarSys::DeleteItem("multiplayer");PerfStats::Done();GameSound::Done();Sound::Done();Input::Done();Console::Done();FontSys::Done();MultiLanguage::Done();EventSys::Done();FileSys::Done();VarSys::Done();TrackSys::Done();}} foundation;
 check(FileSys::AddSrcPack("base",argv[1])&&FileSys::SetActiveStream("base"));
 const auto startup=std::filesystem::path(argv[1]).parent_path().parent_path()/"library/engine/startup.cfg";
 PTree configuration;check(configuration.AddFile(startup.string().c_str()));
 FileSys::ProcessConfigScope(configuration.GetGlobalScope()->GetFunction("GameConfig")->GetFunction("ConfigureFileSystem"));
 MultiLanguage::Configure(configuration.GetGlobalScope()->GetFunction("GameConfig")->GetFunction("ConfigureMultiLanguage"));
 for(unsigned pass=0;pass<1;++pass){
  NativeEngine::InitializeVideoVariables();
  Vid::Command::InitMesh();
  struct MeshCommands {~MeshCommands(){Vid::Command::DoneMesh();}} meshCommands;
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
  check(Vid::Light::Init());
  check(Mesh::Manager::Init());
  struct Resources {bool active=true;~Resources(){if(active){Mesh::Manager::Done();Vid::Light::Done();Material::Manager::Done();Bitmap::Manager::Done();}}} resources;
  check(Vid::device&&Vid::backBmp.GetSurface()&&Vid::viewRect.Width()==W);
  check(Vid::Config::TrilinearOff());
  Vid::InitResources(TRUE);
  Vid::Light::InitResources();
  for(const char *name:{"engine_light_spotcone.tga","engine_light_halo.tga","engine_light_flare.tga"}){
   auto *bitmap=Bitmap::Manager::Find(name);check(bitmap&&bitmap->GetSurface());
  }
  check(Material::Manager::GetMaterial()!=nullptr);
  Camera camera("main");camera.Setup(Vid::viewRect);Vid::curCamera=&camera;
  struct CameraGuard {~CameraGuard(){Vid::curCamera=nullptr;}} cameraGuard;
  check(Terrain::Init());
  struct TerrainGuard {bool active=true;~TerrainGuard(){if(active)Terrain::Done();}} terrain;
  std::cerr<<"Entering IFace::Init\n";IFace::Init();
  Vid::InitIFace();Mesh::Manager::InitIFace();
  GameGod::InitNativeControls();
  IFace::RegisterControlClass("MultiPlayer::SyncList",+[](U32,IControl *parent,U32)->IControl *{return new MultiPlayer::Controls::SyncList(parent);});
  IFace::RegisterControlClass("MultiPlayer::PlayerList",+[](U32,IControl *parent,U32)->IControl *{return new MultiPlayer::Controls::PlayerList(parent);});
  IFace::RegisterControlClass("MultiPlayer::ChatEdit",+[](U32,IControl *parent,U32)->IControl *{return new MultiPlayer::Controls::ChatEdit(parent);});
  IFace::RegisterControlClass("MultiPlayer::PlayerInfo",+[](U32,IControl *parent,U32)->IControl *{return new MultiPlayer::Controls::PlayerInfo(parent);});
  IFace::RegisterControlClass("MultiPlayer::Report",+[](U32,IControl *parent,U32)->IControl *{return new MultiPlayer::Controls::Report(parent);});
  IFace::RegisterControlClass("MultiPlayer::LocalPings",+[](U32,IControl *parent,U32)->IControl *{return new MultiPlayer::Controls::LocalPings(parent);});
  User::Init();check(User::Create(FileIdent(pass?"TestTwo":"TestOne")));std::cerr<<"Entering Game::Init\n";Game::Init();
 Campaigns::Init();Missions::Init();Difficulty::Init();Mods::Init();Sides::Init();SaveGame::Init();GameTime::Init();
 auto *group=Missions::FindGroup("missions\\multiplayer");check(group);
 auto *mission=group->FindMission("2crazed");check(mission);Missions::SetSelected(mission);
 NativeEngine::BeginOfflineSession("2crazed","Commander");
 GameGod::Loader::InitNativeVariables();
 Orders::Init();Sync::Init();Demo::Init();NativeEngine::InitializeRuntimeVariables();
 NativeEngine::InitializeFrameClock(Clock::Time::Ms());
 Game::RC::Init();Game::RC::Set("Load");
 for(unsigned step=0;step<32;++step){
  std::cerr<<"Original RC frame "<<step<<"\n";
  NativeEngine::BeginNativeFrame(Clock::Time::Ms());Game::RC::Process();
  std::this_thread::sleep_for(std::chrono::milliseconds(16));
 }
 check(Game::MissionLoaded());check(GameTime::SimCycle()>0);std::cerr<<"Simulation callbacks completed; cycles "<<GameTime::SimCycle()<<"\n";
 check(Game::RC::In("Sim"));
 if(const char *capture=std::getenv("REIGN_CAPTURE_PATH")){
  std::vector<std::uint8_t> image(W*H*4);renderer.read_presented_rgba(W,H,image.data());
  std::ofstream file(capture,std::ios::binary);file<<"P6\n"<<W<<" "<<H<<"\n255\n";
  for(std::size_t i=0;i<image.size();i+=4)file.write(reinterpret_cast<const char *>(image.data()+i),3);
  check(bool(file));
 }
 Game::RC::Clear();Game::RC::Done();Game::Done();Demo::Done();Sync::Done();Orders::Done();
 NativeEngine::EndOfflineSession();GameTime::Done();SaveGame::Done();Sides::Done();Mods::Done();Difficulty::Done();Missions::Done();Campaigns::Done();User::Done();
  IFace::UnregisterControlClass("MultiPlayer::LocalPings");
  IFace::UnregisterControlClass("MultiPlayer::Report");
  IFace::UnregisterControlClass("MultiPlayer::PlayerInfo");
  IFace::UnregisterControlClass("MultiPlayer::ChatEdit");
  IFace::UnregisterControlClass("MultiPlayer::PlayerList");
  IFace::UnregisterControlClass("MultiPlayer::SyncList");GameGod::DoneNativeControls();Mesh::Manager::DoneIFace();Vid::DoneIFace();IFace::Done();
  presentation.calls=0;
  check(Vid::device->SetTexture(0,nullptr)==S_OK);
  for(auto state:{D3DRENDERSTATE_ZENABLE,D3DRENDERSTATE_FOGENABLE,D3DRENDERSTATE_SPECULARENABLE,D3DRENDERSTATE_ALPHABLENDENABLE,D3DRENDERSTATE_ALPHATESTENABLE})check(Vid::device->SetRenderState(state,0)==S_OK);
  check(Vid::device->SetRenderState(D3DRENDERSTATE_CULLMODE,D3DCULL_NONE)==S_OK);

  check(Vid::device->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1,0)==S_OK);
  check(Vid::device->BeginScene()==S_OK);
  Vid::SetZWriteState(TRUE);check(Vid::SetZWriteState(FALSE));
  reign::LegacyTL vertices[]={{-.5f,-.5f,.5f,1,0xffff0000,0,0,0},{W-.5f,-.5f,.5f,1,0xffff0000,0,1,0},{W-.5f,H-.5f,.5f,1,0xffff0000,0,1,1},{-.5f,H-.5f,.5f,1,0xffff0000,0,0,1}};
  WORD indices[]={0,1,2,0,2,3};
  check(Vid::DrawIndexedPrimitive(PT_TRIANGLELIST,VERTEX_TYPE(D3DFVF_TLVERTEX),vertices,4,indices,6,RS_BLEND_DEF));
  check(Vid::device->EndScene()==S_OK&&Vid::RenderFlush()&&presentation.calls==1);
  std::vector<std::uint8_t> pixels(W*H*4);renderer.read_presented_rgba(W,H,pixels.data());
  const auto center=(H/2*W+W/2)*4; std::cerr<<"Final pixel "<<int(pixels[center])<<","<<int(pixels[center+1])<<","<<int(pixels[center+2])<<"\n";check(pixels[center]>=254&&pixels[center+1]==0&&pixels[center+2]==0);
  Terrain::Done();terrain.active=false;
  Mesh::Manager::Done();Vid::Light::Done();Material::Manager::Done();Bitmap::Manager::Done();resources.active=false;
  NativeEngine::DetachVideo();check(!Vid::RenderFlush()&&!Vid::device&&!Vid::backBmp.GetSurface());
 }
 std::cout<<"Original 2crazed Load/SimInit/Sim callbacks, nonzero simulation cycles, shutdown and GLES presentation passed on host; Android not executed\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
