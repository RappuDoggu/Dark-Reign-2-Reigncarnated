#include "reign/frontend_input.hpp"
#include "reign/menu_commands.hpp"
#include "campaigns.h"
#include "user.h"
#include "game_login.h"
#include "reign/platform.hpp"
#include "reign/profile_storage.hpp"
#include "frontend_configuration_slice.hpp"
#include "reign/frontend_configuration.hpp"
#include "reign/display_options.hpp"
#include "reign/save_load_services.hpp"
#include "reign/sound_settings.hpp"
#include "reign/campaign_selection.hpp"
#include "reign/command_scopes.hpp"
#include "reign/interface_events.hpp"
#include "reign/frontend_frame.hpp"
#include "reign/frontend_effects.hpp"
#include "reign/frontend_font.hpp"
#include "reign/frontend_bitmap.hpp"
#include "reign/bitmap_manager.hpp"
#include "reign/archive.hpp"
#include "reign/engine_file.hpp"
#include "console_slice.hpp"
#include "varfile_slice.hpp"
#include "reign/key_names.hpp"
#include "reign/cursors.hpp"
#include "reign/frontend_draw.hpp"
#include "reign/localization.hpp"
#include "icbutton.h"
#include "icsystembutton.h"
#include "icmonoview.h"
#include "ickeyview.h"
#include "colorbutton.h"
#include "coloreditor.h"
#include "iclistbox.h"
#include "icmenu.h"
#include "icedit.h"
#include "ictabgroup.h"
#include "ictipwindow.h"
#include "native_console_edit.hpp"
#include "console_viewer.h"
#include "iclistslider.h"
#include "icstatic.h"
#include "iface_messagebox.h"
#include <iostream>
#include <cmath>

#include "reign/menu_frontend.hpp"
#include "reign/console_session.hpp"
#include "control_input.hpp"
namespace {
template<ICSystemButton::Function F> IControl *systemButton(U32,IControl *p,U32){return new ICSystemButton(F,p);}
IControl *keyView(U32,IControl *p,U32){return new KeyViewer(p);}
IControl *mono(U32,IControl *p,U32){return new ICMonoView(p);}
IControl *colorButton(U32,IControl *p,U32){return new ColorButton(p);}
IControl *colorEditor(U32,IControl *p,U32){return new ColorEditor(p);}
IControl *consoleViewer(U32,IControl *p,U32){return new ConsoleViewer(p);}
IControl *edit(U32,IControl *p,U32){return new ICEdit(p);}
IControl *tabgroup(U32,IControl *p,U32){return new ICTabGroup(p);}
IControl *tip(U32,IControl *p,U32){return new ICTipWindow(p);}
IControl *consoleEdit(U32,IControl *p,U32){return new ConsoleEdit(p);}
IControl *menu(U32,IControl *p,U32){return new ICMenu(p);}
IControl *listbox(U32,IControl *p,U32){return new ICListBox(p);}
IControl *sliderThumb(U32,IControl *p,U32){return new ICSliderThumb(p);}
IControl *slider(U32,IControl *p,U32){return new ICSlider(p);}
IControl *listSlider(U32,IControl *p,U32){return new ICListSlider(p);}
IControl *button(U32,IControl *p,U32){return new ICButton(p);}
IControl *label(U32,IControl *p,U32){return new ICStatic(p);}
IControl *window(U32,IControl *p,U32){return new ICWindow(p);}
IControl *message(U32,IControl *p,U32){return new MsgBoxWindow(p);}
std::string filename(const std::string &path){auto at=path.find_last_of("/\\");return at==std::string::npos?path:path.substr(at+1);}
}
namespace IFace {Bool InputEventHandler(Event &);void NativeUpdateHover(S32,S32);void NativeClearRouting();void NativeRegisterSubstitution();void NativeUnregisterSubstitution();}
namespace {
bool runtimeOwned=false;
thread_local std::function<void(const std::string &)> *hostCommand=nullptr;
void requestHost(const std::string &action){if(!hostCommand)throw std::runtime_error("Menu host not bound");(*hostCommand)(action);}
void runCodeCommand(U32){const char *name=nullptr;if(Console::ArgCount()!=2||!Console::GetArgString(1,name))throw std::invalid_argument("Expected one run code");requestHost(name);}
class ShellHandler final:public ICWindow {
public:explicit ShellHandler(IControl *p):ICWindow(p){}
 U32 HandleEvent(Event &e) override {
  if(e.type==IFace::EventID()&&e.subType==IFace::NOTIFY){
   if(e.iface.p1==Crc::CalcStr("Options::Enter")){requestHost("options");return TRUE;}
   if(e.iface.p1==Crc::CalcStr("Main::Exit")){auto *v=VarSys::FindVarItem("gamegod.flow.state");requestHost(v&&std::string(v->Str())=="Logout"?"login":"quit");return TRUE;}
  }return ICWindow::HandleEvent(e);
 }
};
struct Globals {
 Globals(){if(runtimeOwned)throw std::logic_error("Original menu globals already owned");TrackSys::Init();VarSys::Init();runtimeOwned=true;}
 ~Globals(){for(auto name:{"iface","multilanguage","console","multiplayer","sys","gamegod"})VarSys::DeleteItem(name);VarSys::Done();TrackSys::Done();runtimeOwned=false;}
};
struct CampaignGlobals {CampaignGlobals(){Campaigns::Init();}~CampaignGlobals(){Campaigns::Done();}};
}
namespace reign {
struct MenuFrontend::Impl {
 Globals globals;
 ConfigLoader loader;
 Localization language{"english"};
 InterfaceMetrics metrics;
 FrontendFrame frame;
 FrontendTheme theme;
 FontRegistry fonts;
 FrontendEffects effects;
 EventLoop loop;
 InterfaceEvents events{loop};
 FrontendInput input{loop};
 BitmapManager bitmaps;
 FrontendTextures textures;
 FontSink fontSink;
 MenuFrameReport report;
 FrontendFonts fontBridge;
 CommandScopes scopes;
 KeyNames keys;
 Cursors cursors;
 SoundLoader soundLoader;
 FrontendConfiguration config;
 ConsoleSession console;
 CampaignGlobals campaigns;
 DisabledAudioSettings audio;
 ProfileStorage profiles;
 std::unique_ptr<ICRoot> root;
 bool closed=false,substitutionRegistered=false;
 std::string pending,action;
 std::function<void(const std::string &)> host=[this](const std::string &s){pending=s;};
 Impl(ConfigLoader load,const std::filesystem::path &privatePath,unsigned width,unsigned height,SoundLoader sound)
 :loader(std::move(load)),metrics(width,height),frame(metrics),fonts(loader),
 bitmaps(loader,[this](const std::string &name){try{loader(name);return true;}catch(const FileSys::FileNotFound &){return false;}}),textures(bitmaps),
 fontBridge(fonts,[this](const NativeFont &font,const FontBatch &batch){if(!fontSink)throw std::runtime_error("Menu font sink not bound");++report.font_batches;fontSink(font,batch);}),
 scopes(loader),soundLoader(std::move(sound)),
 config(theme,fonts,metrics,[this](FScope *s){
  if(s->NameCrc()==Crc::CalcStr("Scancodes")||s->NameCrc()==Crc::CalcStr("Modifiers"))keys.configure(s);
  else if(s->NameCrc()==Crc::CalcStr("Sound"))effects.configure(s,[this](const std::string &name){
   if(soundLoader){auto play=soundLoader(name);return std::function<void(float)>([this,play](float volume){play(volume*audio.digital_volume);});}
   auto bytes=loader(name);if(bytes.empty())throw std::runtime_error("Empty UI sound");
   return std::function<void(float)>([name](float){throw std::runtime_error("Menu audio playback not bound: "+name);});
  });else cursors.configure(s);
 }),console([](VarSys::VarItem *,U32){}),profiles(privatePath,audio){
  {FileSys::Session files(loader);PTree tree;if(!tree.AddFile("english.cfg"))throw std::runtime_error("English database missing");VarFile::Load(tree.GetGlobalScope()->GetFunction("DataBase"),"multilanguage.english");}
  loop.set_handler(input.event_id(),[this](const EngineEvent &e){Event original{};original.type=e.type;original.subType=e.subtype;original.input.code=e.code;original.input.ch=e.character;original.input.state=e.state;original.input.mouseX=e.x;original.input.mouseY=e.y;if(e.subtype>=Input::MOUSEBUTTONDOWN)IFace::NativeUpdateHover(e.x,e.y);return IFace::InputEventHandler(original)!=FALSE;});
  fontBridge.set_frame({{0,0},{int(width),int(height)}},128);frame.set_time({0,0,0});config.register_scopes(scopes);
  try{run([&]{config.initialize_language();root=std::make_unique<ICRoot>();root->SetName("Root");auto &state=iface_event_detail::state();state.root=root.get();state.treeReady=TRUE;
  IFace::RegisterControlClass("Button",button);IFace::RegisterControlClass("Static",label);IFace::RegisterControlClass("Window",window);IFace::RegisterControlClass("MessageBoxWindow",message);
  IFace::RegisterControlClass("Listbox",listbox);IFace::RegisterControlClass("Slider",slider);IFace::RegisterControlClass("ListSlider",listSlider);
  IFace::RegisterControlClass("Menu",menu);
  IFace::RegisterControlClass("Edit",edit);IFace::RegisterControlClass("TabGroup",tabgroup);IFace::RegisterControlClass("TipWindow",tip);IFace::RegisterControlClass("ConsoleEdit",consoleEdit);
  IFace::RegisterControlClass("ConsoleViewer",consoleViewer);
  reign::ProfileStorage::register_login_class();reign::DisplayOptions::register_class();reign::SaveLoadServices::register_class();reign::DisabledAudioSettings::register_class();reign::CampaignSelectionServices::register_classes();
  IFace::RegisterControlClass("ColorButton",colorButton);IFace::RegisterControlClass("ColorEditor",colorEditor);
  IFace::RegisterControlClass("MonoView",mono);IFace::RegisterControlClass("Sys::KeyView",keyView);
  IFace::RegisterControlClass("SysBtnClose",systemButton<ICSystemButton::CLOSE>);
  IFace::RegisterControlClass("SysBtnHelp",systemButton<ICSystemButton::HELP>);
  IFace::RegisterControlClass("SysBtnLeft",systemButton<ICSystemButton::SLIDER_LEFT>);
  IFace::RegisterControlClass("SysBtnRight",systemButton<ICSystemButton::SLIDER_RIGHT>);
  IFace::RegisterControlClass("SysBtnUp",systemButton<ICSystemButton::SLIDER_UP>);
  IFace::RegisterControlClass("SysBtnDown",systemButton<ICSystemButton::SLIDER_DOWN>);
  IFace::RegisterControlClass("SysBtnMin",systemButton<ICSystemButton::MINIMIZE>);
  IFace::RegisterControlClass("SysBtnMax",systemButton<ICSystemButton::MAXIMIZE>);
  IFace::RegisterControlClass("SysBtnDropList",systemButton<ICSystemButton::DROPLIST>);
  IFace::RegisterControlClass("SliderThumb",sliderThumb);

   IFace::NativeRegisterSubstitution();substitutionRegistered=true;register_interface_commands();register_standalone_launch_commands();
   VarSys::RegisterHandler("sys",runCodeCommand,VarSys::NOTIFY);
   VarSys::CreateCmd("sys.runcode",VarSys::NOTIFY);
   VarSys::CreateString("sys.buildtype","Release");
   VarSys::CreateString("sys.buildver","Reigncarnated 0.4 POC");
   VarSys::CreateString("gamegod.flow.state","");
   root->Activate();scopes.execute("runcode_main_login.cfg");
  });}catch(...){destroy_tree();throw;}
 }
 void run(const std::function<void()> &call){
  auto *previous=hostCommand;hostCommand=&host;
  struct Restore{std::function<void(const std::string &)> *p;~Restore(){hostCommand=p;}} restore{previous};
  language.run([&]{cursors.run([&]{events.run([&]{theme.run([&]{frame.run([&]{effects.run([&]{fontBridge.run([&]{textures.run([&]{config.run([&]{keys.run([&]{console.run([&]{profiles.run([&]{input.run(call);});});});});});});});});});});});});
 }
 // Parse the owner's release configuration, retaining the original main-menu
 // definitions and controls. Mission/cinematic/network UI is deliberately not
 // instantiated; its services are not available in this frontend milestone.
 void load_shell(){
  if(IFace::FindByName("Shell::Main",root.get()))return;
  FileSys::Session files(loader);PTree tree;
  if(!tree.AddFile("interface_shell.cfg"))throw std::runtime_error("Import missions/system/shell.zwp to open the main menu");
  FScope *main=nullptr;PTree selected;auto *defs=selected.GetGlobalScope()->AddFunction("ConfigureInterface");
  auto *global=tree.GetGlobalScope();global->InitIterators();
  while(auto *s=global->NextFunction()){
   if(s->NameCrc()==Crc::CalcStr("ConfigureInterface")){
    while(auto *child=s->NextFunction()){
     if(child->NameCrc()!=Crc::CalcStr("CreateColorGroup")&&child->NameCrc()!=Crc::CalcStr("DefineControlType"))continue;
     std::string name=child->NextArgString();child->InitIterators();
     if(name.rfind("ShellMain",0)==0||name.rfind("sys::texture",0)==0||name=="sys::temptextureshell")defs->AddDup(child);
    }
   }else if(s->NameCrc()==Crc::CalcStr("CreateControl")){
    std::string name=s->NextArgString();s->InitIterators();if(name=="Shell::Main")main=s;
   }
  }
  if(!main)throw std::runtime_error("Release Shell::Main missing from shell.zwp");
  defs->InitIterators();IFace::ConfigureNative(defs);
  auto *handler=new ShellHandler(root.get());handler->SetName("Shell::EventHandler");handler->Activate();
  IFace::CreateNativeRootControl(main);
  for(auto name:{"Campaigns","LoadGame","InstantAction","Multi","Credits"}){
   auto path=std::string("Shell::Main.")+name;auto *c=IFace::FindByName(path.c_str(),root.get());
   if(c)c->SetStateItem("Disabled",TRUE);
  }
 }
 void finish_input(){
  if(pending.empty())return;auto next=std::move(pending);pending.clear();
  if(next=="shell"){
   if(!User::LoggedIn())throw std::runtime_error("Select or create a profile first");
   load_shell();
   auto *shell=IFace::FindByName("Shell::Main",root.get());shell->Activate();
   login()->Deactivate();IFace::NativeClearRouting();
   action="Main menu ready. Static backdrop; gameplay, multiplayer and credits are not yet available.";
  }else if(next=="login"){
   User::Logout();auto *shell=IFace::FindByName("Shell::Main",root.get());if(shell)shell->Deactivate();
   IFace::NativeClearRouting();IFace::FindByName("DlgLogin",root.get())->Activate();action="Select a profile to log in.";
  }else if(next=="quit"||next=="options")action="@"+next;
  else throw std::runtime_error("Run code not implemented: "+next);
 }
 void destroy_tree(){
  if(!root)return;
  run([&]{if(root->IsActive())root->Deactivate();IFace::NativeClearRouting();if(substitutionRegistered){IFace::NativeUnregisterSubstitution();substitutionRegistered=false;}root.reset();events.process();auto &s=iface_event_detail::state();s.root=nullptr;s.treeReady=FALSE;});
 }
 ~Impl(){destroy_tree();}
 IControl *login(){auto *result=IFace::FindByName("DlgLogin",root.get());if(!result||!result->IsActive())throw std::runtime_error("Original Login control is not active");return result;}
 void notify(U32 message){Event event{};event.type=IFace::EventID();event.subType=IFace::NOTIFY;event.iface.p1=message;login()->HandleEvent(event);}
};
MenuFrontend::MenuFrontend(ConfigLoader loader,const std::filesystem::path &path,unsigned width,unsigned height,SoundLoader sound):impl_(std::make_unique<Impl>(std::move(loader),path,width,height,std::move(sound))){}
MenuFrontend::~MenuFrontend()=default;
MenuFrameReport MenuFrontend::draw(const GeometrySink &geometry,const FontSink &font,std::uint32_t elapsed,std::uint32_t step){
 if(impl_->closed||!geometry||!font)throw std::invalid_argument("Closed menu or missing draw sinks");
 impl_->report={};impl_->fontSink=font;struct Reset{FontSink &sink;~Reset(){sink={};}} reset{impl_->fontSink};
 impl_->frame.set_time({elapsed,step,float(step)/1000});
 impl_->run([&]{auto *login=IFace::FindByName("DlgLogin",impl_->root.get());auto *backdrop=IFace::FindByName("Backdrop",impl_->root.get());impl_->report.login_active=login&&login->IsActive();impl_->report.backdrop_active=backdrop&&backdrop->IsActive();
  auto paint=impl_->root->GetPaintInfo();FrontendDraw::run_batches([&]{impl_->root->Draw(paint);},[&](const FrontendBatch &batch){++impl_->report.geometry_batches;geometry(batch);});
 });return impl_->report;
}
std::array<int,2> MenuFrontend::control_center(const std::string &name){if(impl_->closed)throw std::logic_error("Menu closed");std::array<int,2> result{};impl_->run([&]{auto *c=IFace::FindByName(name.c_str(),impl_->root.get());if(!c||!c->IsActive())throw std::runtime_error("Control not active: "+name);auto p=c->GetScreenPos();auto size=c->GetSize();result={p.x+size.x/2,p.y+size.y/2};});return result;}
std::string MenuFrontend::profile_name(){if(impl_->closed)throw std::logic_error("Menu closed");std::string name;impl_->run([&]{if(User::LoggedIn())name=User::GetName();});return name;}
bool MenuFrontend::control_active(const std::string &name){if(impl_->closed)return false;bool result=false;impl_->run([&]{auto *control=IFace::FindByName(name.c_str(),impl_->root.get());result=control&&control->IsActive();});return result;}
void MenuFrontend::text(const std::string &ascii){if(impl_->closed)throw std::logic_error("Menu closed");for(unsigned char c:ascii)if(c<32||c>126)throw std::invalid_argument("Initial profile text supports printable ASCII only");impl_->run([&]{for(unsigned char c:ascii){if(!impl_->input.character(c))throw std::runtime_error("Menu input queue full");impl_->events.process();impl_->finish_input();}});}
void MenuFrontend::key(unsigned scan,bool down){if(impl_->closed)throw std::logic_error("Menu closed");impl_->run([&]{if(!impl_->input.key(scan,down))throw std::runtime_error("Menu input queue full");impl_->events.process();impl_->finish_input();});}
void MenuFrontend::pointer_move(int x,int y){if(impl_->closed)throw std::logic_error("Menu closed");impl_->run([&]{if(!impl_->input.move(x,y))throw std::runtime_error("Menu input queue full");impl_->events.process();impl_->finish_input();});}
void MenuFrontend::pointer_button(bool down,int x,int y){if(impl_->closed)throw std::logic_error("Menu closed");impl_->run([&]{if(!impl_->input.button(0,down,x,y))throw std::runtime_error("Menu input queue full");impl_->events.process();impl_->finish_input();});}
void MenuFrontend::execute_config(const std::string &name){if(impl_->closed)throw std::logic_error("Menu closed");impl_->run([&]{impl_->scopes.execute(name);});}
void MenuFrontend::create_profile(const std::string &name){if(impl_->closed)throw std::logic_error("Menu closed");impl_->run([&]{auto *var=VarSys::FindVarItem(impl_->login()->FindVarName("$newUser"));if(!var)throw std::runtime_error("Login variable missing");var->SetStr(name.c_str());impl_->notify(Game::LoginMsg::Create);});}
void MenuFrontend::login_selected_profile(){if(impl_->closed)throw std::logic_error("Menu closed");impl_->run([&]{impl_->notify(Game::LoginMsg::Login);impl_->events.process();impl_->finish_input();});}
std::string MenuFrontend::take_action(){auto result=std::move(impl_->action);impl_->action.clear();return result;}
float MenuFrontend::effect_volume()const{return impl_->audio.digital_volume;}
void MenuFrontend::set_effect_volume(float v){if(!std::isfinite(v)||v<0||v>1)throw std::invalid_argument("Invalid volume");impl_->run([&]{impl_->audio.digital_volume=v;if(User::LoggedIn()&&!User::Save())throw std::runtime_error("Could not save volume");});}
void MenuFrontend::open_shell_preview(){
 if(impl_->closed)throw std::logic_error("Menu closed");
 impl_->run([&]{impl_->load_shell();
  for(auto name:{"DlgLogin","Backdrop"})if(auto *c=IFace::FindByName(name,impl_->root.get()))c->Deactivate();
  auto *shell=IFace::FindByName("Shell::Main",impl_->root.get());shell->Activate();
  for(auto name:{"Campaigns","LoadGame","InstantAction","Multi","Credits","Options","Logout"}){
   auto path=std::string("Shell::Main.")+name;if(auto *c=IFace::FindByName(path.c_str(),impl_->root.get()))c->SetStateItem("Disabled",TRUE);
  }
  IFace::NativeClearRouting();
 });
}
void MenuFrontend::close(){if(impl_->closed)return;impl_->destroy_tree();impl_->profiles.close();impl_->closed=true;}
}
