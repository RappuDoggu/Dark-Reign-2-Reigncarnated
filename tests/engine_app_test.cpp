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
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <memory>
#include <fstream>
extern "C" void __real__ZN7Terrain3Sky6RenderEv();
extern "C" void __wrap__ZN7Terrain3Sky6RenderEv(){if(!std::getenv("REIGN_HIDE_SKY"))__real__ZN7Terrain3Sky6RenderEv();}
extern "C" void __real__ZN10MapObjCtrl6RenderEv();
extern "C" void __wrap__ZN10MapObjCtrl6RenderEv(){if(!std::getenv("REIGN_HIDE_OBJECTS"))__real__ZN10MapObjCtrl6RenderEv();}
extern "C" void __real__ZN6Client7Display6RenderEv();
extern "C" void __wrap__ZN6Client7Display6RenderEv(){if(!std::getenv("REIGN_HIDE_HUD"))__real__ZN6Client7Display6RenderEv();}
namespace Vid {namespace Command {void InitMesh();void DoneMesh();}}
#include "runtime_setup_native.h"
namespace {
constexpr unsigned W=640,H=480;
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
#include "mission_app.h"
#include "viewer.h"
#include "viewer_private.h"
#include "variable_failure.h"
#include "load_status.h"
#include "input_native.h"
#include "multiplayer_data.h"
#include "player.h"
namespace Client {UnitObj *FirstSelected();}
int main(int argc,char **argv){try{
 if(argc<2||argc>5)throw std::runtime_error("Provide installation [mission [group [campaign]]]");
 const std::string selectedMission=argc>2?argv[2]:"2crazed";NativeEngine::MissionOptions options;if(argc>3)options.group=argv[3];options.campaign=argc>4&&std::string(argv[4])=="campaign";
 auto argument=std::filesystem::path(argv[1]);auto root=std::filesystem::is_directory(argument)?argument:argument.parent_path().parent_path();
 auto work=std::filesystem::temp_directory_path()/("reign-app-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 Context context;reign::LegacyGles renderer(resolve);Presentation presentation{&renderer};
 for(unsigned pass=0;pass<2;++pass){
  NativeEngine::MissionApp app(root.string(),work.string(),selectedMission,renderer,present,&presentation,options);
  if(options.campaign)check(!MultiPlayer::Data::HaveData()&&Difficulty::GetApplyEnemy());
  // Original optional-texture lookup must return no texture, not throw before
  // FindCreate can apply its own extension check (retail J01 first-frame failure).
  check(Bitmap::Manager::Find("")==nullptr);
  check(Bitmap::Manager::FindCreate(Bitmap::reduceNONE,"")==nullptr);
  bool rejected=false;try{NativeEngine::MissionApp duplicate(root.string(),work.string(),selectedMission,renderer,present,&presentation,options);}catch(const std::logic_error &){rejected=true;}check(rejected);
  for(unsigned frame=0;frame<(pass?64u:128u);++frame){app.Frame();if(!app.SimulationCycles())check(NativeEngine::missionLoadStatus.Report().rfind("READY\n",0)!=0);std::this_thread::sleep_for(std::chrono::milliseconds(16));}
  check(app.InSimulation()&&app.SimulationCycles()>0);
  if(options.campaign)check(Player::GetCurrentPlayer()&&Player::GetCurrentPlayer()->GetTeam()->IsDefaultClient());
  if(pass==0&&argc==2){
   auto *unit=Team::GetDisplayTeam()->GetUnitObjects().GetHead();check(unit);
   Vector point=unit->Position();point.y+=3;F32 rhw;Vid::ProjectFromWorld(point,rhw);
   std::cerr<<"Picking starting rig at "<<point.x<<","<<point.y<<"\n";
   NativeEngine::QueuePointer(S32(point.x),S32(point.y),-1,FALSE);app.Frame();
   NativeEngine::QueuePointer(S32(point.x),S32(point.y),0,TRUE);app.Frame();
   NativeEngine::QueuePointer(S32(point.x),S32(point.y),0,FALSE);app.Frame();app.Frame();
   std::cerr<<"Selected unit "<<(Client::FirstSelected()?Client::FirstSelected()->UnitType()->GetName():"none")<<"\n";
   check(Client::FirstSelected()!=nullptr);
   auto *moving=Client::FirstSelected();Vector start=moving->Position();Vector destination=start+Vector(16,0,16);destination.y=Terrain::FindFloor(destination.x,destination.z);Vid::ProjectFromWorld(destination,rhw);
   NativeEngine::QueuePointer(S32(destination.x),S32(destination.y),-1,FALSE);app.Frame();
   NativeEngine::QueuePointer(S32(destination.x),S32(destination.y),0,TRUE);app.Frame();
   NativeEngine::QueuePointer(S32(destination.x),S32(destination.y),0,FALSE);app.Frame();
   for(unsigned frame=0;frame<96;++frame){app.Frame();std::this_thread::sleep_for(std::chrono::milliseconds(16));}
   auto end=moving->Position();float dx=end.x-start.x,dz=end.z-start.z;std::cerr<<"Move-order displacement "<<dx<<","<<dz<<"\n";check(dx*dx+dz*dz>1.0f);

  }
  if(std::getenv("REIGN_CINEMATIC_FIXTURE")){
   // Authored fixture: exercise the original timeline, fade, letterbox, input,
   // shroud and interface restoration without redistributing campaign data.
   PTree timeline;auto *cin=timeline.GetGlobalScope()->AddFunction("Cineractive");
   auto *start=cin->AddFunction("At");start->AddArgFPoint(0);
   for(const char *name:{"DisableIFace","DisableInput","DisableShroud"})start->AddFunction(name)->AddArgInteger(1);
   auto *letter=start->AddFunction("Letterbox");letter->AddFunction("Direction")->AddArgString("Down");auto *time=letter->AddFunction("Time");time->AddArgFPoint(0.1f);time->AddArgFPoint(1.5f);time->AddArgFPoint(1.7f);
   auto *fade=start->AddFunction("Fade");fade->AddFunction("Direction")->AddArgString("Down");time=fade->AddFunction("Time");time->AddArgFPoint(0.2f);time->AddArgFPoint(1.7f);time->AddArgFPoint(1.9f);
   if(std::getenv("REIGN_MESH_CINEMATIC_FIXTURE")){
    // Load an uncached real text mesh through the same cinematic primitive as
    // the J01 stack. Keep all owned bytes in this private temporary runtime.
    auto *source=FileSys::Open("engine_plane.xsi");check(source&&source->Size()>0);
    const auto path=work/"recovery_cinematic_mesh.xsi";
    {std::ofstream out(path,std::ios::binary);out.write(static_cast<const char *>(source->GetMemoryPtr()),source->Size());check(bool(out));}FileSys::Close(source);
    auto *at=cin->AddFunction("At");at->AddArgFPoint(0.3f);auto *mesh=at->AddFunction("Mesh");mesh->AddFunction("Godfile")->AddArgString(path.c_str());
    auto *offset=mesh->AddFunction("Offset");offset->AddArgFPoint(0);offset->AddArgFPoint(0);offset->AddArgFPoint(3);
    time=mesh->AddFunction("Time");time->AddArgFPoint(0.1f);time->AddArgFPoint(1.4f);time->AddArgFPoint(1.6f);
   }
   auto *end=cin->AddFunction("At");end->AddArgFPoint(2.0f);end->AddFunction("EndCineractive");
   Viewer::Action::Execute(Team::GetDisplayTeam(),cin);
   for(unsigned frame=0;frame<150;++frame){app.Frame();
    if(frame==45){std::vector<std::uint8_t> image(W*H*4);renderer.read_rgba(W,H,image.data());unsigned visible=0;for(unsigned y=80;y<400;++y)for(unsigned x=0;x<W;++x){auto p=(y*W+x)*4;if(image[p]||image[p+1]||image[p+2])++visible;}check(visible>100);check(!image[0]&&!image[1]&&!image[2]);}
    if(frame==55){check(Viewer::SetCurrent("Viewer::Action::Cinematic",TRUE));auto *cinema=dynamic_cast<Viewer::CinemaCam *>(Viewer::GetCurrent());check(cinema);cinema->curveMatrix=Vid::CurCamera().WorldMatrix();check(!app.CanPan());bool locked=false;try{app.Control(5,1,1);}catch(const std::logic_error &){locked=true;}check(locked);}
    std::this_thread::sleep_for(std::chrono::milliseconds(16));}
   check(app.CanPan());app.Control(5,1,1);
   check(renderer.diagnostics().find("DX7 Clear:")==std::string::npos&&renderer.diagnostics().find("DX7 SetViewport:")==std::string::npos);
   check(!Viewer::Action::IsPlaying());
   VarInteger unbound;bool rejectedVar=false;try{(void)*unbound;}catch(const std::runtime_error &){rejectedVar=true;}
   check(rejectedVar&&NativeEngine::LastVariableFailure().find("VarInteger::operator*")!=std::string::npos&&NativeEngine::LastVariableFailure().find("+0x")!=std::string::npos);
   NativeEngine::LastVariableFailure().clear();
   check(Vid::Var::Terrain::shroud.Alive()&&*Vid::Var::Terrain::shroud==1);
   std::cerr<<"Original cineractive timeline, fade/letterbox, shroud/input/HUD restoration passed\n";
  }
  auto before=app.SimulationCycles();NativeEngine::QueueKey(0xc8,TRUE);app.Frame();before=app.SimulationCycles();app.SetActive(false);check(!Input::IsKeyDown(0xc8));
  for(unsigned frame=0;frame<4;++frame)app.Frame();check(app.SimulationCycles()==before);
  app.SetActive(true);for(unsigned frame=0;frame<20;++frame){app.Frame();std::this_thread::sleep_for(std::chrono::milliseconds(16));}
  check(app.SimulationCycles()>before);
  std::cerr<<"Application lifecycle pass "<<pass<<" cycles "<<app.SimulationCycles()<<" presented "<<presentation.calls<<"\n";
  std::cerr<<app.Diagnostics()<<"\n"<<renderer.diagnostics()<<"\n";
 if(const char *capture=std::getenv("REIGN_CAPTURE_PATH")){
  std::vector<std::uint8_t> image(W*H*4);renderer.read_presented_rgba(W,H,image.data());
  std::ofstream file(capture,std::ios::binary);file<<"P6\n"<<W<<" "<<H<<"\n255\n";
  for(std::size_t i=0;i<image.size();i+=4)file.write(reinterpret_cast<const char *>(image.data()+i),3);
  check(bool(file));
 }

  if(std::getenv("REIGN_NAVIGATION_FIXTURE")){
   Missions::NextMission(TRUE);app.Frame();auto route=app.TakeNavigation();check(route&&!route->menu&&route->campaign&&route->mission=="Followup");check(!app.TakeNavigation());
   auto destination=*route;app.Close();check(!Vid::isStatus.initialized&&!Vid::device);
   auto followOptions=options;followOptions.group=destination.group;followOptions.campaign=destination.campaign;
   NativeEngine::MissionApp follow(root.string(),work.string(),destination.mission,renderer,present,&presentation,followOptions);
   for(unsigned frame=0;frame<64;++frame){follow.Frame();std::this_thread::sleep_for(std::chrono::milliseconds(16));}check(follow.InSimulation()&&follow.SimulationCycles()>0&&follow.CanPan());
   Missions::NextMission(TRUE);follow.Frame();auto ending=follow.TakeNavigation();check(ending&&ending->menu);follow.Close();
   std::cerr<<"Original campaign NextMission -> deferred Followup load -> final menu route passed\n";
  }
  app.Close();check(!Vid::isStatus.initialized&&!Vid::device);
 }
 std::filesystem::remove_all(work);
 std::cout<<"Shared native mission application: load, simulation, pause/resume, shutdown and profile login passed twice on host; no Android execution\n";
 return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
