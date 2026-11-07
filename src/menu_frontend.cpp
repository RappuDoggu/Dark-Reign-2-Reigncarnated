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
#include "reign/instant_setup_layout.hpp"
#include "game_preview.h"
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
IControl *windowTitle(U32,IControl *p,U32){return new ICWindowTitle(p);}
IControl *message(U32,IControl *p,U32){return new MsgBoxWindow(p);}
std::string filename(const std::string &path){auto at=path.find_last_of("/\\");return at==std::string::npos?path:path.substr(at+1);}
}
#include "reign/host_options.hpp"
namespace IFace {Bool InputEventHandler(Event &);void NativeUpdateHover(S32,S32);void NativeClearRouting();void NativeRegisterSubstitution();void NativeUnregisterSubstitution();}
namespace {
bool runtimeOwned=false;
thread_local std::function<void(const std::string &)> *hostCommand=nullptr;
void requestHost(const std::string &action){if(!hostCommand)throw std::runtime_error("Menu host not bound");(*hostCommand)(action);}
class InstantSetupWindow final:public ICWindow {
public:explicit InstantSetupWindow(IControl *p):ICWindow(p){}
 U32 HandleEvent(Event &e)override{
  if(e.type==IFace::EventID()&&e.subType==IFace::NOTIFY){
   for(const auto &entry:{std::pair<const char *,const char *>{"NativeSetup::AI","setup-ai"},{"NativeSetup::Difficulty","setup-difficulty"},{"NativeSetup::Back","setup-back"},{"NativeSetup::Launch","setup-launch"},{"NativeSetup::Map","setup-map"},{"NativeSetup::MapDone","setup-map-done"}})
    if(e.iface.p1==Crc::CalcStr(entry.first)){requestHost(entry.second);return TRUE;}
  }
  return ICWindow::HandleEvent(e);
 }
};
IControl *instantSetupWindow(U32,IControl *p,U32){return new InstantSetupWindow(p);}
unsigned selectionNumber(const std::string &s,unsigned maximum,const char *field,bool positive=false){
 if(s.empty()||s.size()>10)throw std::invalid_argument(std::string("Invalid ")+field);
 std::uint64_t value=0;for(unsigned char c:s){if(c<'0'||c>'9')throw std::invalid_argument(std::string("Invalid ")+field);value=value*10+c-'0';}
 if(value>maximum||(positive&&value==0))throw std::invalid_argument(std::string(field)+" is out of range");return unsigned(value);
}
void runCodeCommand(U32 id){if(id!=Crc::CalcStr("sys.runcode"))return;const char *name=nullptr;if(Console::ArgCount()!=2||!Console::GetArgString(1,name))throw std::invalid_argument("Expected one run code");requestHost(name);}
void instantCommand(U32 id){
 const char *target=nullptr;
 if(id!=Crc::CalcStr("gamegod.verify")||Console::ArgCount()!=2||!Console::GetArgString(1,target)||(std::string(target)!="Shell::Main.InstantAction"&&std::string(target)!="Shell::Main.Campaigns"))throw std::invalid_argument("Unsupported shell host command");
 // The original button requests this command before entering Instant Action.
 // Route it to the available native mission chooser after event dispatch.
 requestHost(std::string(target)=="Shell::Main.Campaigns"?"campaign":"instant");
}
class ShellHandler final:public ICWindow {
public:explicit ShellHandler(IControl *p):ICWindow(p){}
 U32 HandleEvent(Event &e) override {
  if(e.type==IFace::EventID()&&e.subType==IFace::NOTIFY){
   if(e.iface.p1==Crc::CalcStr("Campaigns::Enter")){requestHost("campaign");return TRUE;}
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
 std::vector<MenuMission> instantMaps;
 std::vector<std::string> instantDifficulties;
 std::function<ConfigLoader(const MenuMission &)> missionLoader;
 std::unique_ptr<Game::Preview> instantPreview;
 unsigned previewMap=~0u;
 unsigned instantOpponents=0,instantDifficulty=0;
 bool closed=false,substitutionRegistered=false;
 std::string pending,action;
 HostOptionsRequest optionsRequest;
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
  IFace::RegisterControlClass("Native::InstantSetup",instantSetupWindow);
  IFace::RegisterControlClass("WindowTitle",windowTitle);
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
   // Only commands notify these handlers. Display/flow variables must not
   // inherit command notifications when changed without console arguments.
   VarSys::RegisterHandler("sys",runCodeCommand);
   VarSys::CreateCmd("sys.runcode",VarSys::NOTIFY);
   VarSys::RegisterHandler("gamegod",instantCommand);VarSys::CreateCmd("gamegod.verify",VarSys::NOTIFY);
   VarSys::CreateString("sys.buildtype","Release");
   VarSys::CreateString("sys.buildver","Reigncarnated native");
   VarSys::CreateString("gamegod.flow.state","");
   root->Activate();scopes.execute("runcode_main_login.cfg");
  });}catch(...){destroy_tree();throw;}
 }
 void run(const std::function<void()> &call){
  HostOptionsBinding optionsBinding(optionsRequest);
  auto *previous=hostCommand;hostCommand=&host;
  struct Restore{std::function<void(const std::string &)> *p;~Restore(){hostCommand=p;}} restore{previous};
  language.run([&]{cursors.run([&]{events.run([&]{theme.run([&]{frame.run([&]{effects.run([&]{fontBridge.run([&]{textures.run([&]{config.run([&]{keys.run([&]{console.run([&]{profiles.run([&]{input.run(call);});});});});});});});});});});});});
  if(optionsRequest.take())action="@options";
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
  for(auto name:{"LoadGame","Multi","Credits"}){
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
  }else if(next=="setup-ai"){if(missionLoader&&!instantOpponents&&instantPreview&&instantPreview->GetTeams()<2)throw std::runtime_error("Selected map has no second playable starting team");instantOpponents=1-instantOpponents;setup_labels();}
  else if(next=="setup-difficulty"){instantDifficulty=(instantDifficulty+1)%instantDifficulties.size();setup_labels();if(missionLoader){previewMap=~0u;refresh_preview();}}
  else if(next=="setup-map")control("GameConfigHostOptions")->Activate();
  else if(next=="setup-map-done"){refresh_preview();control("GameConfigHostOptions")->Deactivate();IFace::NativeClearRouting();}
  else if(next=="setup-back")action="@instant-back";
  else if(next=="setup-launch"){selection();action="@instant-launch";}
  else if(next=="quit"||next=="options"||next=="instant"||next=="campaign")action="@"+next;
  else throw std::runtime_error("Run code not implemented: "+next);
 }
 std::string control_name(const std::string &name){if(!missionLoader)return name;for(const auto &entry:{std::pair<const char *,const char *>{"Maps","GameConfigHostOptions.Maps"},{"Opponents","GameSetup::Instant.GameConfigPlayers.AddAi"},{"Difficulty","GameSetup::Instant.GameConfigPlayers.Difficulty"},{"Seed","GameSetup::Instant.GameConfigPlayers.Seed"},{"Launch","GameSetup::Instant.GameConfigMenu.Launch"},{"Back","GameSetup::Instant.GameConfigMenu.Back"}}){auto prefix=std::string("GameSetup::Instant.")+entry.first;if(name==prefix||name.rfind(prefix+".",0)==0)return entry.second+name.substr(prefix.size());}return name;}
 IControl *control(const char *name){auto path=control_name(name);auto *c=IFace::FindByName(path.c_str(),root.get());if(!c)throw std::runtime_error(std::string("Missing setup control: ")+path);return c;}
 void setup_labels(){
  control("GameSetup::Instant.Opponents")->SetTextString(Utils::Ansi2Unicode(missionLoader?(instantOpponents?"Remove AI":"Add AI"):(instantOpponents?"One AI opponent":"Practice: no opponent")),TRUE);
  if(missionLoader)control("GameSetup::Instant.GameConfigPlayers.Opponent")->SetTextString(Utils::Ansi2Unicode(instantOpponents?"Computer opponent":"Practice: no opponent"),TRUE);
  const auto &name=instantDifficulties.at(instantDifficulty);auto label=std::string("Difficulty: ")+(name.empty()?"Mission default":name);
  control("GameSetup::Instant.Difficulty")->SetTextString(Utils::Ansi2Unicode(label.c_str()),TRUE);
 }
 void refresh_preview(){
  if(!missionLoader)return;
  auto *map=VarSys::FindVarItem("gamegod.instant.map");if(!map)throw std::logic_error("Map selection variable is missing");unsigned index=selectionNumber(std::string(map->Str()),unsigned(instantMaps.size()-1),"Map selection");if(index==previewMap)return;
  const auto selected=instantMaps.at(index);CampaignSelectionServices services;services.open_mission=[&](const std::string &,const std::string &){return missionLoader(selected);};services.default_side=[]{return "jda";};
  std::unique_ptr<Game::Preview> preview;services.run([&]{preview=std::make_unique<Game::Preview>(selected.group.c_str(),selected.name.c_str());});
  if(instantOpponents&&preview->GetTeams()<2)throw std::runtime_error("Selected map has fewer than two playable starting teams. Remove the AI or choose another map.");
  auto *image=control("GameSetup::Instant.GameConfigMission.Preview");image->SetImage(nullptr);instantPreview=std::move(preview);if(auto *bitmap=instantPreview->GetTerrainTexture()){TextureInfo texture(bitmap,TextureInfo::TM_CENTRED);image->SetImage(&texture);}
  auto set=[&](const char *child,const std::string &text){control((std::string("GameSetup::Instant.GameConfigMission.Info.")+child).c_str())->SetTextString(Utils::Ansi2Unicode(text.c_str()),TRUE);};
  set("MissionName",selected.name);set("MissionSize",std::to_string(instantPreview->GetSize())+" cells / "+std::to_string(instantPreview->GetTeams())+" starting teams");
  set("RuleSet",std::string("Rules: ")+(instantPreview->GetRuleSet().str[0]?instantPreview->GetRuleSet().str:"mission default"));set("StartingResource","Credits: mission default");set("UnitLimit","Unit limit: mission default");set("Placement","Placement: original starting teams");set("Difficulty",instantDifficulties.at(instantDifficulty).empty()?"Difficulty: mission default":instantDifficulties.at(instantDifficulty));set("ResourceType","Resources: mission default");set("Filtering","Original mission rules");previewMap=index;
 }
 InstantSetupSelection selection(){
  if(instantMaps.empty()||instantDifficulties.empty())throw std::logic_error("Instant Action setup was not prepared");
  refresh_preview();
  // Original ICEdit buffers text until Enter/focus loss. Commit it before
  // validation so clicking Launch never uses the previously saved seed.
  auto *edit=control("GameSetup::Instant.Seed");if(edit->IsActive()){Event enter{};enter.type=IFace::EventID();enter.subType=IFace::NOTIFY;enter.iface.p1=ICEditMsg::Enter;edit->HandleEvent(enter);}
  auto *map=VarSys::FindVarItem("gamegod.instant.map"),*seed=VarSys::FindVarItem("gamegod.instant.seed");
  if(!map||!seed)throw std::logic_error("Instant Action variables are missing");
  unsigned index=selectionNumber(std::string(map->Str()),unsigned(instantMaps.size()-1),"Map selection");
  return {instantMaps[index],index,instantOpponents,selectionNumber(std::string(seed->Str()),2147483647,"Random seed",true),instantDifficulty,instantDifficulties.at(instantDifficulty)};
 }
 void create_setup(){
  if(IFace::FindByName("GameSetup::Instant",root.get()))return;
  if(missionLoader){
   auto cfg=original_instant_layout(loader);FileSys::Session files([&](const std::string &name){if(name=="native-original-instant.cfg")return std::vector<std::uint8_t>(cfg.begin(),cfg.end());return loader(name);});PTree tree;if(!tree.AddFile("native-original-instant.cfg"))throw std::runtime_error("Cannot construct original Instant Action controls");
   auto *global=tree.GetGlobalScope();global->InitIterators();while(auto *scope=global->NextFunction()){if(scope->NameCrc()==Crc::CalcStr("ConfigureInterface"))IFace::ConfigureNative(scope);else if(scope->NameCrc()==Crc::CalcStr("CreateControl"))IFace::CreateNativeRootControl(scope);else throw std::runtime_error("Unsupported native setup root scope");}return;
  }
  // Use the original control renderer, input, fonts and theme. Only options
  // already applied by the native offline session are exposed here.
  static const std::string cfg=R"(
CreateControl("GameSetup::Instant","Native::InstantSetup"){
 Size(640,480);Style("Transparent","NoAutoActivate","!DropShadow");
 CreateControl("Title","Static"){Pos(42,58);Size(550,32);Font("WindowTitle");Style("Transparent");Text("Instant Action");}
 CreateControl("MapLabel","Static"){Pos(42,94);Size(310,20);Font("System");Style("Transparent");Text("Map");}
 CreateControl("Maps","Listbox"){
  Pos(42,118);Size(318,250);Style("VSlider");UseVar("gamegod.instant.map");
  ItemConfig(){Font("System");Geometry("AutoSizeY","ParentWidth");Size(0,9);}
 }
 CreateControl("OpponentLabel","Static"){Pos(382,94);Size(218,20);Font("System");Style("Transparent");Text("Opponent");}
 CreateControl("Opponents","Button"){Pos(382,118);Size(218,42);Font("System");TranslateEvent("Button::Notify::Pressed","NativeSetup::AI","<");}
 CreateControl("Difficulty","Button"){Pos(382,182);Size(218,42);Font("System");TranslateEvent("Button::Notify::Pressed","NativeSetup::Difficulty","<");}
 CreateControl("SeedLabel","Static"){Pos(382,246);Size(218,20);Font("System");Style("Transparent");Text("Random seed");}
 CreateControl("Seed","Edit"){Pos(382,272);Size(218,38);Font("System");UseVar("gamegod.instant.seed");}
 CreateControl("Hint","Static"){Pos(42,380);Size(558,20);Font("System");Style("Transparent");Text("Choose a map and opponent, then launch.");}
 CreateControl("Back","Button"){Pos(42,414);Size(140,40);Font("System");Text("Back");TranslateEvent("Button::Notify::Pressed","NativeSetup::Back","<");}
 CreateControl("Launch","Button"){Pos(460,414);Size(140,40);Font("System");Text("Launch");TranslateEvent("Button::Notify::Pressed","NativeSetup::Launch","<");}
})";
  FileSys::Session files([this](const std::string &name){if(name=="native-instant-setup.cfg")return std::vector<std::uint8_t>(cfg.begin(),cfg.end());return loader(name);});
  PTree tree;if(!tree.AddFile("native-instant-setup.cfg"))throw std::runtime_error("Cannot construct native Instant Action controls");
  IFace::CreateNativeRootControl(tree.GetGlobalScope()->GetFunction("CreateControl"));
 }
 void destroy_tree(){
  if(!root)return;
  run([&]{if(root->IsActive())root->Deactivate();IFace::NativeClearRouting();if(substitutionRegistered){IFace::NativeUnregisterSubstitution();substitutionRegistered=false;}root.reset();instantPreview.reset();events.process();auto &s=iface_event_detail::state();s.root=nullptr;s.treeReady=FALSE;});
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
std::array<int,2> MenuFrontend::control_center(const std::string &name){if(impl_->closed)throw std::logic_error("Menu closed");std::array<int,2> result{};impl_->run([&]{auto *c=IFace::FindByName(impl_->control_name(name).c_str(),impl_->root.get());if(!c||!c->IsActive())throw std::runtime_error("Control not active: "+name);auto p=c->GetScreenPos();auto size=c->GetSize();result={p.x+size.x/2,p.y+size.y/2};});return result;}
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
void MenuFrontend::set_build_version(const std::string &version){if(version.empty()||version.size()>128)throw std::invalid_argument("Invalid APK version label");impl_->run([&]{VarSys::FindVarItem("sys.buildver")->SetStr(version.c_str());});}
void MenuFrontend::use_original_instant_setup(std::function<ConfigLoader(const MenuMission &)> loader){if(!loader||impl_->closed)throw std::invalid_argument("Missing original setup mission loader");impl_->missionLoader=std::move(loader);}
void MenuFrontend::prepare_instant_setup(const std::vector<MenuMission> &maps,const std::vector<std::string> &difficulties,unsigned map,unsigned opponents,unsigned difficulty,unsigned seed){
 if(impl_->closed)throw std::logic_error("Menu closed");
 if(maps.empty())throw std::runtime_error("No Instant Action maps were found. Import the complete missions folder.");
 if(difficulties.empty()||!difficulties.front().empty()||opponents>1||seed==0||seed>2147483647)throw std::invalid_argument("Invalid Instant Action defaults");
 for(const auto &value:difficulties)if(value.find_first_of("\t\r\n")!=std::string::npos)throw std::invalid_argument("Invalid difficulty name");
 impl_->run([&]{impl_->instantMaps=maps;impl_->instantDifficulties=difficulties;impl_->instantOpponents=opponents;impl_->instantDifficulty=std::min<unsigned>(difficulty,difficulties.size()-1);
  if(!VarSys::FindVarItem("gamegod.instant.map"))VarSys::CreateString("gamegod.instant.map","0");
  if(!VarSys::FindVarItem("gamegod.instant.seed"))VarSys::CreateString("gamegod.instant.seed","1");
  const auto index=std::min<unsigned>(map,maps.size()-1);
  VarSys::FindVarItem("gamegod.instant.map")->SetStr(std::to_string(index).c_str());VarSys::FindVarItem("gamegod.instant.seed")->SetStr(std::to_string(seed).c_str());
  impl_->previewMap=~0u;impl_->create_setup();auto *list=dynamic_cast<ICListBox *>(impl_->control("GameSetup::Instant.Maps"));if(!list)throw std::runtime_error("Invalid map list control");
  list->DeleteAllItems();for(unsigned i=0;i<maps.size();++i){auto label=std::filesystem::path(maps[i].group).filename().string()+" / "+maps[i].name;list->AddTextItem(std::to_string(i).c_str(),Utils::Ansi2Unicode(label.c_str()));}
  list->SetSelectedItem(index,TRUE,FALSE);impl_->setup_labels();impl_->refresh_preview();
 });
}
void MenuFrontend::hide_shell_controls(){if(impl_->closed)throw std::logic_error("Menu closed");impl_->run([&]{for(auto *name:{"Shell::Main","GameSetup::Instant","GameConfigHostOptions"})if(auto *c=IFace::FindByName(name,impl_->root.get()))c->Deactivate();IFace::NativeClearRouting();});}
void MenuFrontend::show_instant_setup(){if(impl_->closed)throw std::logic_error("Menu closed");impl_->run([&]{if(impl_->instantMaps.empty())throw std::logic_error("Instant Action setup is empty");impl_->control("Shell::Main")->Deactivate();impl_->control("GameSetup::Instant")->Activate();IFace::NativeClearRouting();});}
void MenuFrontend::show_main(){if(impl_->closed)throw std::logic_error("Menu closed");impl_->run([&]{for(auto *name:{"GameSetup::Instant","GameConfigHostOptions"})if(auto *c=IFace::FindByName(name,impl_->root.get()))c->Deactivate();impl_->control("Shell::Main")->Activate();IFace::NativeClearRouting();});}
InstantSetupSelection MenuFrontend::instant_selection(){if(impl_->closed)throw std::logic_error("Menu closed");InstantSetupSelection result;impl_->run([&]{result=impl_->selection();});return result;}
bool MenuFrontend::text_input_active(){if(impl_->closed)return false;bool result=false;impl_->run([&]{result=dynamic_cast<ICEdit *>(IFace::GetFocus())!=nullptr;});return result;}
void MenuFrontend::open_shell_preview(){
 if(impl_->closed)throw std::logic_error("Menu closed");
 impl_->run([&]{impl_->load_shell();
  for(auto name:{"DlgLogin","Backdrop"})if(auto *c=IFace::FindByName(name,impl_->root.get()))c->Deactivate();
  auto *shell=IFace::FindByName("Shell::Main",impl_->root.get());shell->Activate();
  for(auto name:{"LoadGame","Multi","Credits","Logout"}){
   auto path=std::string("Shell::Main.")+name;if(auto *c=IFace::FindByName(path.c_str(),impl_->root.get()))c->SetStateItem("Disabled",TRUE);
  }
  IFace::NativeClearRouting();
 });
}
void MenuFrontend::close(){if(impl_->closed)return;impl_->destroy_tree();impl_->profiles.close();impl_->closed=true;}
}

