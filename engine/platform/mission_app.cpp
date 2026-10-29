#include "vid_private.h"
#include "mission_app.h"
#include "input_native.h"
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

#include "light_priv.h"
#include "mesh.h"
#include "camera.h"
#include "terrain_priv.h"
#include "worldctrl.h"
#include "team.h"
namespace Sight {class Map;}
#include "sight.h"
#include "unitobj.h"
#include "game_config.h"
#include "filesys.h"
#include "tracksys.h"
#include "varsys.h"
#include "vid_native.h"
#include "surface_gles.h"
#include "device_gles.h"
#include "reign/legacy_gles.hpp"

#include <functional>
namespace Vid {namespace Command {void InitMesh();void DoneMesh();}}
#include "runtime_setup_native.h"
namespace NativeEngine {
namespace {bool engineActive=false,enginePoisoned=false,clockReady=false;
void Require(bool ok,const char *what){if(!ok)throw std::runtime_error(what);}
template<class T> void Register(const char *name){IFace::RegisterControlClass(name,+[](U32,IControl *parent,U32)->IControl *{return new T(parent);});}
}
struct MissionApp::Impl {
 std::vector<std::function<void()>> cleanup;
 std::unique_ptr<Camera> camera;
 bool closed=false,active=true,pausedHere=false,owns=false,gameInitialized=false;
 void EndGame(){if(gameInitialized){Game::Done();gameInitialized=false;}}
 void Defer(std::function<void()> fn){cleanup.push_back(std::move(fn));}
 void Stop(){
  if(closed)return;closed=true;if(!owns)return;std::exception_ptr first;
  while(!cleanup.empty()){auto fn=std::move(cleanup.back());cleanup.pop_back();try{fn();}catch(...){if(!first)first=std::current_exception();}}
  engineActive=false;if(first){enginePoisoned=true;std::rethrow_exception(first);}
 }
 ~Impl(){try{Stop();}catch(const std::exception &e){std::fprintf(stderr,"Mission shutdown: %s\n",e.what());}}
 void Start(const std::string &installation,const std::string &writable,const std::string &mission,reign::LegacyGles &renderer,PresentVideo present,void *context){
  if(engineActive||enginePoisoned)throw std::logic_error("Original engine is already active or requires a process restart");
  if(!present)throw std::invalid_argument("Missing presentation callback");
  auto root=std::filesystem::canonical(installation);auto work=std::filesystem::absolute(writable);
  Require(std::filesystem::exists(root/"library/engine/startup.cfg"),"Import the complete installation, including library/engine/startup.cfg");
  auto pack=std::filesystem::exists(root/"packs/base.zwp")?root/"packs/base.zwp":root/"base.zwp";
  Require(std::filesystem::exists(pack),"Missing owned base.zwp");
  std::filesystem::create_directories(work);auto previous=std::filesystem::current_path();
  engineActive=true;owns=true;Defer([previous]{std::filesystem::current_path(previous);});
  for(const char *name:{"missions","worlds","sides","mods"})if(std::filesystem::exists(root/name)&&!std::filesystem::exists(work/name))std::filesystem::create_directory_symlink(root/name,work/name);
  if(!std::filesystem::exists(work/"packs")){
   std::filesystem::create_directory(work/"packs");
   if(std::filesystem::is_directory(root/"packs"))for(const auto &entry:std::filesystem::directory_iterator(root/"packs"))std::filesystem::create_symlink(entry.path(),work/"packs"/entry.path().filename());
  }
  if(!std::filesystem::exists(work/"packs/base.zwp"))std::filesystem::create_symlink(pack,work/"packs/base.zwp");
  std::filesystem::current_path(work);
  if(!clockReady){Clock::Time::Init();clockReady=true;}
  TrackSys::Init();Defer([]{TrackSys::Done();});
  VarSys::Init();Defer([]{for(auto *name:{"vid","gamegod","sys","multiplayer"})VarSys::DeleteItem(name);VarSys::Done();});
  FileSys::Init();Defer([]{FileSys::Done();});EventSys::Init();Defer([]{EventSys::Done();});
  MultiLanguage::Init();Defer([]{MultiLanguage::Done();});FontSys::Init();Defer([]{FontSys::Done();});
  Console::Init();Defer([]{Console::Done();});Input::Init(nullptr,nullptr);Defer([]{Input::Done();});SetInputViewport(640,480);
  Sound::Init();Defer([]{Sound::Done();});GameSound::Init();Defer([]{GameSound::Done();});PerfStats::Init();Defer([]{PerfStats::Done();});
  Require(FileSys::AddSrcPack("base",pack.string().c_str())&&FileSys::SetActiveStream("base"),"Cannot mount base.zwp");
  PTree configuration;Require(configuration.AddFile((root/"library/engine/startup.cfg").string().c_str()),"Cannot read startup.cfg");
  auto *config=configuration.GetGlobalScope()->GetFunction("GameConfig");
  FileSys::ProcessConfigScope(config->GetFunction("ConfigureFileSystem"));MultiLanguage::Configure(config->GetFunction("ConfigureMultiLanguage"));
  InitializeVideoVariables();Vid::Command::InitMesh();Defer([]{Vid::Command::DoneMesh();});
  Vid::Heap::Init(2048,6144);Defer([]{Vid::Heap::Done();});Vid::InitBuckets();Defer([]{Vid::DoneBuckets();});
  Require(AttachGlesVideo(renderer,640,480,present,context)==S_OK,"Cannot attach GLES video");Defer([]{DetachVideo();});
  Require(Bitmap::Manager::Init(),"Cannot initialize bitmaps");Defer([]{Bitmap::Manager::Done();});
  Require(Material::Manager::Init(),"Cannot initialize materials");Defer([]{Material::Manager::Done();});
  Require(Vid::Light::Init(),"Cannot initialize lighting");Defer([]{Vid::Light::Done();});
  Require(Mesh::Manager::Init(),"Cannot initialize meshes");Defer([]{Mesh::Manager::Done();});
  Vid::InitResources(TRUE);Vid::Light::InitResources();
  camera=std::make_unique<Camera>("main");camera->Setup(Vid::viewRect);Vid::curCamera=camera.get();Defer([this]{Vid::curCamera=nullptr;camera.reset();});
  Require(Terrain::Init(),"Cannot initialize terrain");Defer([]{Terrain::Done();});
  IFace::Init();Defer([]{IFace::Done();});Vid::InitIFace();Defer([]{Vid::DoneIFace();});Mesh::Manager::InitIFace();Defer([]{Mesh::Manager::DoneIFace();});
  GameGod::InitNativeControls();Defer([]{GameGod::DoneNativeControls();});
  Register<MultiPlayer::Controls::SyncList>("MultiPlayer::SyncList");Register<MultiPlayer::Controls::PlayerList>("MultiPlayer::PlayerList");
  Register<MultiPlayer::Controls::ChatEdit>("MultiPlayer::ChatEdit");Register<MultiPlayer::Controls::PlayerInfo>("MultiPlayer::PlayerInfo");
  Register<MultiPlayer::Controls::Report>("MultiPlayer::Report");Register<MultiPlayer::Controls::LocalPings>("MultiPlayer::LocalPings");
  Defer([]{for(auto *name:{"MultiPlayer::LocalPings","MultiPlayer::Report","MultiPlayer::PlayerInfo","MultiPlayer::ChatEdit","MultiPlayer::PlayerList","MultiPlayer::SyncList"})IFace::UnregisterControlClass(name);});
  User::Init();Defer([]{User::Done();});
  FileIdent user("Commander");if(auto *existing=User::Find(user))Require(User::Login(existing),"Cannot log in to local profile");else Require(User::Create(user),"Cannot create local profile");
  Game::Init();gameInitialized=true;Defer([this]{EndGame();});
  Campaigns::Init();Defer([]{Campaigns::Done();});Missions::Init();Defer([]{Missions::Done();});Difficulty::Init();Defer([]{Difficulty::Done();});
  Mods::Init();Defer([]{Mods::Done();});Sides::Init();Defer([]{Sides::Done();});SaveGame::Init();Defer([]{SaveGame::Done();});GameTime::Init();Defer([]{GameTime::Done();});
  auto *group=Missions::FindGroup("missions\\multiplayer");Require(group!=nullptr,"No multiplayer mission group was imported");
  auto *selected=group->FindMission(mission.c_str());Require(selected!=nullptr,"Requested Instant Action map is missing");Missions::SetSelected(selected);
  BeginOfflineSession(mission,"Commander");Defer([]{EndOfflineSession();});GameGod::Loader::InitNativeVariables();
  Orders::Init();Defer([]{Orders::Done();});Sync::Init();Defer([]{Sync::Done();});Demo::Init();Defer([]{Demo::Done();});InitializeRuntimeVariables();
  Defer([this]{EndGame();});
  InitializeFrameClock(Clock::Time::Ms());Game::RC::Init();Defer([]{Game::RC::Clear();Game::RC::Done();});Game::RC::Set("Load");
 }
};
MissionApp::MissionApp(const std::string &installation,const std::string &writable,const std::string &mission,reign::LegacyGles &renderer,PresentVideo present,void *context):impl(std::make_unique<Impl>()){
 try{impl->Start(installation,writable,mission,renderer,present,context);}catch(...){if(impl->owns)enginePoisoned=true;throw;}
}
MissionApp::~MissionApp()=default;
void MissionApp::Close(){impl->Stop();}
void MissionApp::Frame(){if(impl->closed)throw std::logic_error("Mission runtime is closed");if(BeginNativeFrame()){Game::RC::Process();Sound::Poll();}}
void MissionApp::SetActive(bool active){
 if(impl->closed||impl->active==active)return;
 if(!active){impl->pausedHere=!GameTime::Paused();if(impl->pausedHere)GameTime::Pause(FALSE);}
 else {if(impl->pausedHere&&GameTime::Paused())GameTime::Pause(FALSE);GameTime::Readjust();impl->pausedHere=false;}
 Input::OnActivate(active?TRUE:FALSE);
 SetFrameActive(active?TRUE:FALSE,Clock::Time::Ms());impl->active=active;
}
bool MissionApp::InSimulation() const{return !impl->closed&&Game::RC::In("Sim");}
std::uint32_t MissionApp::SimulationCycles() const{return impl->closed?0:GameTime::SimCycle();}
}
