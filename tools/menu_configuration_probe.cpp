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
int main(int argc,char **argv){try{
 if(argc<2||argc>4)throw std::invalid_argument("Usage: menu_configuration_probe base.zwp [root.cfg] [--exercise-login]");
 if(argc==4&&std::string(argv[3])!="--exercise-login")throw std::invalid_argument("Unknown probe option");
 reign::Archive archive(argv[1]);auto loader=[&](const std::string &name){auto key=filename(name);if(!archive.find(key))throw FileSys::FileNotFound(key);return archive.read(key);};
 struct Session{Session(){TrackSys::Init();VarSys::Init();}~Session(){VarSys::Done();TrackSys::Done();}} session;
 {FileSys::Session files(loader);PTree tree;if(!tree.AddFile("english.cfg"))throw std::runtime_error("English database missing");auto *db=tree.GetGlobalScope()->GetFunction("DataBase");VarFile::Load(db,"multilanguage.english");}
 reign::Localization language("english");
 reign::InterfaceMetrics metrics(640,480);reign::FrontendFrame frame(metrics);frame.set_time({0,0,0});
 reign::FrontendTheme theme;reign::FontRegistry fonts(loader);reign::FrontendEffects effects;reign::InterfaceEvents events;
 reign::BitmapManager bitmaps(loader,[&](const std::string &name){return archive.find(filename(name))!=nullptr;});reign::FrontendTextures textures(bitmaps);
 bool profileCreated=false;bool loginActive=false,backdropActive=false;unsigned geometryBatches=0;unsigned glyphs=0,consoleDisplays=0,cursorBatches=0;reign::FrontendFonts fontBridge(fonts,[&](const auto &,const auto &){++glyphs;});fontBridge.set_frame({{0,0},{640,480}},128);
 reign::CommandScopes scopes(loader);reign::KeyNames keys;reign::Cursors cursors;reign::FrontendConfiguration config(theme,fonts,metrics,[&](FScope *s){if(s->NameCrc()==Crc::CalcStr("Scancodes")||s->NameCrc()==Crc::CalcStr("Modifiers"))keys.configure(s);else if(s->NameCrc()==Crc::CalcStr("Sound"))effects.configure(s,[&](const std::string &name){auto bytes=loader(name);if(bytes.empty())throw std::runtime_error("Empty UI sound");return [name](float){throw std::runtime_error("Audio playback not available in configuration probe: "+name);};});else cursors.configure(s);});config.register_scopes(scopes);
 std::string blocker;unsigned classes=0;
 language.run([&]{cursors.run([&]{config.initialize_language();events.run([&]{theme.run([&]{frame.run([&]{effects.run([&]{fontBridge.run([&]{textures.run([&]{config.run([&]{
  ICRoot root;root.SetName("Root");auto &state=reign::iface_event_detail::state();state.root=&root;state.treeReady=TRUE;
  struct Cleanup{ICRoot &r;reign::iface_event_detail::State &s;~Cleanup(){if(r.IsActive())r.Deactivate();s.root=nullptr;s.treeReady=FALSE;}} cleanup{root,state};
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
  root.Activate();
  try{
   auto profileRoot=std::filesystem::temp_directory_path()/("reign-menu-profile-"+std::to_string(reign::monotonic_ns()));
   struct ProfileCleanup{std::filesystem::path p;~ProfileCleanup(){std::filesystem::remove_all(p);}} profileCleanup{profileRoot};
   Campaigns::Init();struct CampaignCleanup{~CampaignCleanup(){Campaigns::Done();}} campaignCleanup;
   reign::DisabledAudioSettings audio;reign::ProfileStorage profiles(profileRoot,audio);
   reign::register_interface_commands();reign::register_standalone_launch_commands();profiles.run([&]{reign::run_console([&]{scopes.execute(argc>=3?argv[2]:"runcode_main_login.cfg");
    if(argc==4){
     auto *dialog=IFace::FindByName("DlgLogin",&root);if(!dialog||!dialog->IsActive())throw std::runtime_error("Login not active for event test");
     auto *newName=VarSys::FindVarItem(dialog->FindVarName("$newUser"));if(!newName)throw std::runtime_error("Login newUser variable missing");newName->SetStr("Native Probe Pilot");
     Event event{};event.type=IFace::EventID();event.subType=IFace::NOTIFY;event.iface.p1=Game::LoginMsg::Create;dialog->HandleEvent(event);
     profileCreated=User::Find(FileIdent("Native Probe Pilot"))!=nullptr;if(!profileCreated)throw std::runtime_error("Original Login Create event failed");
     event.iface.p1=Game::LoginMsg::Login;dialog->HandleEvent(event);
    }},[&](VarSys::VarItem *,U32){++consoleDisplays;});});profiles.close();}
  catch(const std::exception &e){blocker=e.what();}
  auto *login=IFace::FindByName("DlgLogin",&root);auto *backdrop=IFace::FindByName("Backdrop",&root);
  loginActive=login&&login->IsActive();backdropActive=backdrop&&backdrop->IsActive();
  if(loginActive&&backdropActive){auto paint=root.GetPaintInfo();reign::FrontendDraw::run_batches([&]{root.Draw(paint);},[&](const auto &){++geometryBatches;});}
  if(cursors.select(nullptr))reign::FrontendDraw::run_batches([&]{cursors.draw(100,100,16);},[&](const auto &){++cursorBatches;});
  classes=state.ctrlTypes.GetCount();root.Deactivate();events.process();
 });});});});});});});});});
 std::cout<<"Original PTree/Main/IFace configuration attempt: "<<(blocker.empty()?"COMPLETED":"BLOCKED")<<"\nInterface statements attempted: "<<config.processed()<<"\nRegistered classes: "<<classes<<"\nFont aliases: "<<fonts.alias_count()<<"\nBitmaps: "<<bitmaps.count()<<"\n";
 std::cout<<"Cursors: "<<cursors.count()<<" cursor batches: "<<cursorBatches<<'\n';
 std::cout<<"Configured key names: "<<keys.key_count()<<" modifiers: "<<keys.modifier_count()<<'\n';
 if(!blocker.empty())std::cout<<"Blocking dependency: "<<blocker<<'\n';
 std::cout<<"Login active: "<<loginActive<<" backdrop active: "<<backdropActive<<" geometry batches: "<<geometryBatches<<" glyph batches: "<<glyphs<<'\n';
 std::cout<<"Scripted Login Create event: "<<(argc==4?(profileCreated?"PASS":"FAIL"):"NOT REQUESTED")<<'\n';
 std::cout<<"Full Login/Shell lifecycle, GPU screen rendering, Android APK: NOT demonstrated\n";
 VarSys::DeleteItem("iface");VarSys::DeleteItem("multilanguage");VarSys::DeleteItem("multiplayer");
 return blocker.empty()?0:2;
}catch(const std::exception &e){std::cerr<<"MENU_CONFIG_PROBE_FAIL "<<e.what()<<'\n';return 1;}}
