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
#include <sstream>
#include <locale>

#include "light_priv.h"
#include "mesh.h"
#include "camera.h"
#include "terrain_priv.h"
#include "worldctrl.h"
#include "gameconstants.h"
#include "team.h"
#include "viewer.h"
#include "viewer_private.h"
#include "client_private.h"
#include <cmath>
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
#include "load_status.h"

#include <functional>
namespace Vid {namespace Command {void InitMesh();void DoneMesh();}}
#include "runtime_setup_native.h"
#include "resource_links_native.h"
namespace NativeEngine {
namespace {bool engineActive=false,enginePoisoned=false,clockReady=false;
void Require(bool ok,const char *what){if(!ok)throw std::runtime_error(what);}
template<class T> void Register(const char *name){IFace::RegisterControlClass(name,+[](U32,IControl *parent,U32)->IControl *{return new T(parent);});}
}
struct MissionApp::Impl {
 std::vector<std::function<void()>> cleanup;
 std::unique_ptr<Camera> camera;
 bool closed=false,active=true,pausedHere=false,owns=false,gameInitialized=false,reportedSimulation=false;
 void EndGame(){if(gameInitialized){Game::Done();gameInitialized=false;}}
 void Defer(std::function<void()> fn){cleanup.push_back(std::move(fn));}
 void Stop(){
  if(closed)return;closed=true;if(!owns)return;std::exception_ptr first;
  while(!cleanup.empty()){auto fn=std::move(cleanup.back());cleanup.pop_back();try{fn();}catch(...){if(!first)first=std::current_exception();}}
  engineActive=false;if(first){enginePoisoned=true;std::rethrow_exception(first);}
 }
 ~Impl(){try{Stop();}catch(const std::exception &e){std::fprintf(stderr,"Mission shutdown: %s\n",e.what());}}
 void Start(const std::string &installation,const std::string &writable,const std::string &mission,reign::LegacyGles &renderer,PresentVideo present,void *context,const MissionOptions &options){
  if(engineActive||enginePoisoned)throw std::logic_error("Original engine is already active or requires a process restart");
  if(!present)throw std::invalid_argument("Missing presentation callback");
  auto root=std::filesystem::canonical(installation);auto work=std::filesystem::absolute(writable);
  Require(std::filesystem::exists(root/"library/engine/startup.cfg"),"Import the complete installation, including library/engine/startup.cfg");
  auto pack=std::filesystem::exists(root/"packs/base.zwp")?root/"packs/base.zwp":root/"base.zwp";
  Require(std::filesystem::exists(pack),"Missing owned base.zwp");
  std::filesystem::create_directories(work);auto previous=std::filesystem::current_path();
  engineActive=true;owns=true;missionLoadStatus.Reset();missionLoadStatus.Update("Mounting installation and startup configuration");Defer([previous]{std::filesystem::current_path(previous);});
  PrepareResourceLinks(root,work,pack);
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
  missionLoadStatus.Update("Initializing video and textures");InitializeVideoVariables();Vid::Command::InitMesh();Defer([]{Vid::Command::DoneMesh();});
  Vid::Heap::Init(2048,6144);Defer([]{Vid::Heap::Done();});Vid::InitBuckets();Defer([]{Vid::DoneBuckets();});
  Require(AttachGlesVideo(renderer,640,480,present,context)==S_OK,"Cannot attach GLES video");Defer([]{DetachVideo();});
  Require(Bitmap::Manager::Init(),"Cannot initialize bitmaps");Defer([]{Bitmap::Manager::Done();});
  Require(Material::Manager::Init(),"Cannot initialize materials");Defer([]{Material::Manager::Done();});
  Require(Vid::Light::Init(),"Cannot initialize lighting");Defer([]{Vid::Light::Done();});
  missionLoadStatus.Update("Initializing meshes, terrain and interface");Require(Mesh::Manager::Init(),"Cannot initialize meshes");Defer([]{Mesh::Manager::Done();});
  // Original High detail and unreduced 32-bit textures. Keep mission lighting
  // and sight rules intact; unsupported stencil/multitexture remain explicit.
  Vid::renderState.textureReduction=0;
  for(const auto &setting:{std::pair<const char *,int>{"mesh.mrm.active",0},{"mesh.mrm.auto.active",0},{"mesh.show.multiweight",1},{"mesh.show.interpolation",1},{"mesh.show.overlay",1},{"mesh.show.envmap",1},{"mesh.light.quick",0},{"mesh.light.single",0}}){auto *item=VarSys::FindVarItem(setting.first);Require(item!=nullptr,"Missing high quality graphics setting");item->SetInteger(setting.second);}
  Vid::InitResources(TRUE);Vid::Light::InitResources();
  camera=std::make_unique<Camera>("main");camera->Setup(Vid::viewRect);Vid::curCamera=camera.get();Defer([this]{Vid::curCamera=nullptr;camera.reset();});
  Require(Terrain::Init(),"Cannot initialize terrain");Defer([]{Terrain::Done();});
  IFace::Init();Defer([]{IFace::Done();});Vid::InitIFace();Defer([]{Vid::DoneIFace();});Mesh::Manager::InitIFace();Defer([]{Mesh::Manager::DoneIFace();});
  GameGod::InitNativeControls();Defer([]{GameGod::DoneNativeControls();});
  Register<MultiPlayer::Controls::SyncList>("MultiPlayer::SyncList");Register<MultiPlayer::Controls::PlayerList>("MultiPlayer::PlayerList");
  Register<MultiPlayer::Controls::ChatEdit>("MultiPlayer::ChatEdit");Register<MultiPlayer::Controls::PlayerInfo>("MultiPlayer::PlayerInfo");
  Register<MultiPlayer::Controls::Report>("MultiPlayer::Report");Register<MultiPlayer::Controls::LocalPings>("MultiPlayer::LocalPings");
  Defer([]{for(auto *name:{"MultiPlayer::LocalPings","MultiPlayer::Report","MultiPlayer::PlayerInfo","MultiPlayer::ChatEdit","MultiPlayer::PlayerList","MultiPlayer::SyncList"})IFace::UnregisterControlClass(name);});
  missionLoadStatus.Update("Loading local profile and mission catalog");User::Init();Defer([]{User::Done();});
  FileIdent user("Commander");if(auto *existing=User::Find(user))Require(User::Login(existing),"Cannot log in to local profile");else Require(User::Create(user),"Cannot create local profile");
  Game::Init();gameInitialized=true;Defer([this]{EndGame();});
  Campaigns::Init();Defer([]{Campaigns::Done();});
  // Save while campaign progress is alive, including a failed startup.
  Defer([this]{EndGame();if(User::LoggedIn()){bool saved=User::Save();User::Logout();Require(saved,"Could not save local campaign profile");}});
  Missions::Init();Defer([]{Missions::Done();});Difficulty::Init();Defer([]{Difficulty::Done();});if(!options.difficulty.empty()){auto *setting=Difficulty::FindSetting(GameIdent(options.difficulty.c_str()));Require(setting!=nullptr,"Unknown imported difficulty setting");Difficulty::SetCurrentSetting(setting->GetName());}
  Difficulty::SetApplyEnemy(options.campaign?TRUE:FALSE);
  Mods::Init();Defer([]{Mods::Done();});Sides::Init();Defer([]{Sides::Done();});SaveGame::Init();Defer([]{SaveGame::Done();});GameTime::Init();Defer([]{GameTime::Done();});
  std::string groupPath=options.group;std::replace(groupPath.begin(),groupPath.end(),'/','\\');auto *group=Missions::FindGroup(groupPath.c_str());Require(group!=nullptr,"Selected mission group was not imported");
  auto *selected=group->FindMission(mission.c_str());Require(selected!=nullptr,"Selected mission is missing from the imported group");if(options.campaign&&!Campaigns::Find(selected))throw std::invalid_argument("Selected mission is not part of an imported campaign");if(!options.campaign&&!group->InstantAction())throw std::invalid_argument("Selected group is not available for Instant Action");Missions::SetSelected(selected);
  BeginOfflineSession(mission,"Commander",options.campaign,options.opponents,options.seed,options.difficulty);Defer([]{EndOfflineSession();});GameGod::Loader::InitNativeVariables();
  Orders::Init();Defer([]{Orders::Done();});Sync::Init();Defer([]{Sync::Done();});Demo::Init();Defer([]{Demo::Done();});InitializeRuntimeVariables();
  Defer([this]{EndGame();});
  InitializeFrameClock(Clock::Time::Ms());Game::RC::Init();Defer([]{Game::RC::Clear();Game::RC::Done();});Game::RC::Set("Load");
 }
};
MissionApp::MissionApp(const std::string &installation,const std::string &writable,const std::string &mission,reign::LegacyGles &renderer,PresentVideo present,void *context,const MissionOptions &options):impl(std::make_unique<Impl>()){
 try{impl->Start(installation,writable,mission,renderer,present,context,options);}catch(...){if(impl->owns)enginePoisoned=true;throw;}
}
MissionApp::~MissionApp()=default;
std::string MissionApp::Diagnostics()const{std::ostringstream out;if(auto *session=CurrentOfflineSession())out<<"mission="<<session->mission<<" campaign="<<session->campaign<<" opponents="<<session->opponents<<" seed="<<session->seed<<" difficulty="<<(session->difficulty.empty()?"mission default":session->difficulty)<<"\n";out<<"placing="<<Placing()<<" camera="<<(Viewer::GetCurrent()?Viewer::GetCurrent()->GetName():"none")<<"\n";out<<"simulation cycles="<<SimulationCycles()<<" display team="<<(Team::GetDisplayTeam()?"set":"missing");if(InSimulation())for(U32 id=0;id<Game::MAX_TEAMS;++id)if(auto *team=Team::Id2Team(id))if(team->IsAvailablePlay())out<<"\nplayable team="<<team->GetName()<<" ai="<<team->IsAI()<<" strategic="<<(team->GetStrategicObject()?"set":"none")<<" personality="<<team->GetPersonality();if(impl->camera){const auto &m=impl->camera->WorldMatrix();out<<"\nCamera position "<<m.posit.x<<' '<<m.posit.y<<' '<<m.posit.z<<" front "<<m.front.x<<' '<<m.front.y<<' '<<m.front.z;}
 out<<"\nGraphics: high detail, full mesh, 32-bit textures; filters="<<Vid::renderState.status.filter<<" distance fog="<<Vid::renderState.status.fog<<" fog rgb="<<unsigned(Vid::renderState.fogColor.r)<<','<<unsigned(Vid::renderState.fogColor.g)<<','<<unsigned(Vid::renderState.fogColor.b)<<" ambient rgb="<<unsigned(Vid::renderState.ambientColor.r)<<','<<unsigned(Vid::renderState.ambientColor.g)<<','<<unsigned(Vid::renderState.ambientColor.b)<<" terrain shroud="<<(VarSys::FindVarItem("terrain.shroud.active")?VarSys::FindVarItem("terrain.shroud.active")->Integer():-1);
 out<<"\n"<<missionLoadStatus.Report();return out.str();}
void MissionApp::Close(){impl->Stop();}
void MissionApp::Control(unsigned action,float x,float y){
 if(!InSimulation())throw std::logic_error("Mission is still loading");
 if(!std::isfinite(x)||!std::isfinite(y))throw std::invalid_argument("Invalid camera gesture");
 if(action==1){Client::Events::TriggerClientMode(Client::CM_NONE);Client::Events::HandleDiscreteEvent(nullptr,Crc::CalcStr("de::deselect"),0,0);}
 else if(action==2||action==3)Client::Events::HandleDiscreteEvent(nullptr,Crc::CalcStr(action==2?"de::prevdir":"de::nextdir"),0,0);
 else if(action==6)Vid::SetFogStateI(!Vid::renderState.status.fog); // Distance fog only; sight/shroud remain original.
 else if(action==5){auto *camera=dynamic_cast<Viewer::Standard *>(Viewer::GetCurrent());if(!camera)throw std::logic_error("Map pan requires the Standard game camera");const auto &matrix=camera->GetMatrix();Vector right(matrix.right.x,0,matrix.right.z),front(matrix.front.x,0,matrix.front.z);if(right.Magnitude2()<1e-8f||front.Magnitude2()<1e-8f)throw std::logic_error("Current camera has no ground-plane pan direction");right.Normalize();front.Normalize();auto delta=right*std::clamp(x,-32.f,32.f)+front*std::clamp(y,-32.f,32.f);camera->PanNative(delta.x,delta.z);}
 else throw std::invalid_argument("Unknown mission control");
}
void MissionApp::Frame(){if(impl->closed)throw std::logic_error("Mission runtime is closed");if(BeginNativeFrame()){Game::RC::Process();Sound::Poll();if(!impl->reportedSimulation&&InSimulation()){missionLoadStatus.Complete();impl->reportedSimulation=true;}}}
void MissionApp::SetActive(bool active){
 if(impl->closed||impl->active==active)return;
 if(!active){impl->pausedHere=!GameTime::Paused();if(impl->pausedHere)GameTime::Pause(FALSE);}
 else {if(impl->pausedHere&&GameTime::Paused())GameTime::Pause(FALSE);GameTime::Readjust();impl->pausedHere=false;}
 Input::OnActivate(active?TRUE:FALSE);
 SetFrameActive(active?TRUE:FALSE,Clock::Time::Ms());impl->active=active;
}
bool MissionApp::InSimulation() const{return !impl->closed&&Game::RC::In("Sim");}
bool MissionApp::Placing() const{return InSimulation()&&Client::data.clientMode==Client::CM_CONSTRUCT;}
std::uint32_t MissionApp::SimulationCycles() const{return impl->closed?0:GameTime::SimCycle();}
}
