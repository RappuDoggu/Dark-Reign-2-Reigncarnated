#include "frontend_configuration_slice.hpp"
#include "reign/frontend_configuration.hpp"
#include "reign/display_options.hpp"
#include "reign/save_load_services.hpp"
#include "reign/sound_settings.hpp"
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
IControl *colorButton(U32,IControl *p,U32){return new ColorButton(p);}
IControl *colorEditor(U32,IControl *p,U32){return new ColorEditor(p);}
IControl *consoleViewer(U32,IControl *p,U32){return new ConsoleViewer(p);}
IControl *edit(U32,IControl *p,U32){return new ICEdit(p);}
IControl *tabgroup(U32,IControl *p,U32){return new ICTabGroup(p);}
IControl *tip(U32,IControl *p,U32){return new ICTipWindow(p);}
IControl *consoleEdit(U32,IControl *p,U32){return new ConsoleEdit(p);}
IControl *menu(U32,IControl *p,U32){return new ICMenu(p);}
IControl *listbox(U32,IControl *p,U32){return new ICListBox(p);}
IControl *slider(U32,IControl *p,U32){return new ICSlider(p);}
IControl *listSlider(U32,IControl *p,U32){return new ICListSlider(p);}
IControl *button(U32,IControl *p,U32){return new ICButton(p);}
IControl *label(U32,IControl *p,U32){return new ICStatic(p);}
IControl *window(U32,IControl *p,U32){return new ICWindow(p);}
IControl *message(U32,IControl *p,U32){return new MsgBoxWindow(p);}
std::string filename(const std::string &path){auto at=path.find_last_of("/\\");return at==std::string::npos?path:path.substr(at+1);}
}
int main(int argc,char **argv){try{
 if(argc<2||argc>3)throw std::invalid_argument("Usage: menu_configuration_probe base.zwp [root.cfg]");
 reign::Archive archive(argv[1]);auto loader=[&](const std::string &name){auto key=filename(name);if(!archive.find(key))throw FileSys::FileNotFound(key);return archive.read(key);};
 struct Session{Session(){TrackSys::Init();VarSys::Init();}~Session(){VarSys::Done();TrackSys::Done();}} session;
 {FileSys::Session files(loader);PTree tree;if(!tree.AddFile("english.cfg"))throw std::runtime_error("English database missing");auto *db=tree.GetGlobalScope()->GetFunction("DataBase");VarFile::Load(db,"multilanguage.english");}
 reign::Localization language("english");
 reign::InterfaceMetrics metrics(640,480);reign::FrontendFrame frame(metrics);frame.set_time({0,0,0});
 reign::FrontendTheme theme;reign::FontRegistry fonts(loader);reign::FrontendEffects effects;reign::InterfaceEvents events;
 reign::BitmapManager bitmaps(loader,[&](const std::string &name){return archive.find(filename(name))!=nullptr;});reign::FrontendTextures textures(bitmaps);
 unsigned glyphs=0,consoleDisplays=0,cursorBatches=0;reign::FrontendFonts fontBridge(fonts,[&](const auto &,const auto &){++glyphs;});fontBridge.set_frame({{0,0},{640,480}},128);
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
  reign::DisplayOptions::register_class();reign::SaveLoadServices::register_class();reign::DisabledAudioSettings::register_class();
  IFace::RegisterControlClass("ColorButton",colorButton);IFace::RegisterControlClass("ColorEditor",colorEditor);
  root.Activate();
  try{reign::run_console([&]{scopes.execute(argc==3?argv[2]:"runcode_main_login.cfg");},[&](VarSys::VarItem *,U32){++consoleDisplays;});}
  catch(const std::exception &e){blocker=e.what();}
  if(cursors.select(nullptr))reign::FrontendDraw::run_batches([&]{cursors.draw(100,100,16);},[&](const auto &){++cursorBatches;});
  classes=state.ctrlTypes.GetCount();root.Deactivate();events.process();
 });});});});});});});});});
 std::cout<<"Original PTree/Main/IFace configuration attempt: "<<(blocker.empty()?"COMPLETED":"BLOCKED")<<"\nInterface statements attempted: "<<config.processed()<<"\nRegistered classes: "<<classes<<"\nFont aliases: "<<fonts.alias_count()<<"\nBitmaps: "<<bitmaps.count()<<"\n";
 std::cout<<"Cursors: "<<cursors.count()<<" cursor batches: "<<cursorBatches<<'\n';
 std::cout<<"Configured key names: "<<keys.key_count()<<" modifiers: "<<keys.modifier_count()<<'\n';
 if(!blocker.empty())std::cout<<"Blocking dependency: "<<blocker<<'\n';
 std::cout<<"Original Login/Shell lifecycle, screen rendering, Android APK: NOT demonstrated\n";
 VarSys::DeleteItem("iface");VarSys::DeleteItem("multilanguage");
 return blocker.empty()?0:2;
}catch(const std::exception &e){std::cerr<<"MENU_CONFIG_PROBE_FAIL "<<e.what()<<'\n';return 1;}}
