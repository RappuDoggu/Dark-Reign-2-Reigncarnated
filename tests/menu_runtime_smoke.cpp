#include "reign/menu_frontend.hpp"
#include "reign/engine_file.hpp"
#include "reign/platform.hpp"
#include "font_fixture.hpp"
#include <iostream>
int main(){try{
 unsigned checks=0;auto check=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Menu runtime check "+std::to_string(checks));};
 auto loader=[](const std::string &name){
  if(name=="fixture.fff")return font_fixture();
  std::string text;
  if(name=="english.cfg")text="DataBase(){}";
  else if(name=="show.cfg")text="Cmd(\"iface.activate DlgLogin\");";
  else if(name=="runcode_main_login.cfg")text=R"(
ConfigureInterface(){CreateFont("System"){File("fixture.fff");}CreateFont("WindowTitle"){File("fixture.fff");}}
CreateControl("Backdrop","Window"){Size(640,480);CreateControl("HideLogin","Button"){Pos(400,300);Size(100,40);Text("AB");OnEvent("Button::Notify::Pressed"){Deactivate("|DlgLogin");}}}
CreateControl("DlgLogin","Game::Login"){OnEvent("Game::Login::Notify::LoggedIn"){Cmd("sys.runcode shell");}Size(300,250);CreateControl("CreatePilot","Button"){Pos(20,200);Size(80,30);OnEvent("Button::Notify::Pressed"){Op("$<.newUser","=","AB");SendNotifyEvent("|DlgLogin","Game::Login::Message::Create");}}CreateControl("UserList","Listbox"){Size(200,150);UseVar("$<.user");}}
Cmd("iface.activate Backdrop");Cmd("iface.activate DlgLogin");Cmd("multiplayer.login");
)";
  else throw FileSys::FileNotFound(name);
  return std::vector<std::uint8_t>(text.begin(),text.end());
 };
 auto root=std::filesystem::temp_directory_path()/("reign-menu-smoke-"+std::to_string(reign::monotonic_ns()));
 struct Cleanup{std::filesystem::path p;~Cleanup(){std::filesystem::remove_all(p);}} cleanup{root};
 for(unsigned pass=0;pass<2;++pass){reign::MenuFrontend menu(loader,root,640,480);
  for(unsigned i=0;i<3;++i){auto report=menu.draw([](const auto &){},[](const auto &,const auto &){},i*16,16);check(report.login_active&&report.backdrop_active&&report.geometry_batches>0);}
  if(!pass){auto point=menu.control_center("DlgLogin.CreatePilot");menu.pointer_move(point[0],point[1]);menu.pointer_button(true,point[0],point[1]);menu.pointer_button(false,point[0],point[1]);}
  check(menu.profile_name()=="AB");
  bool rejected=false;try{menu.draw([](const auto &){throw std::runtime_error("Intentional sink failure");},[](const auto &,const auto &){},48,16);}catch(const std::exception &){rejected=true;}check(rejected);
  check(menu.draw([](const auto &){},[](const auto &,const auto &){},64,16).login_active);
  menu.pointer_move(410,310);menu.pointer_button(true,410,310);menu.pointer_button(false,410,310);
  check(!menu.draw([](const auto &){},[](const auto &,const auto &){},80,16).login_active);
  menu.execute_config("show.cfg");check(menu.draw([](const auto &){},[](const auto &,const auto &){},96,16).login_active);
  menu.close();rejected=false;try{menu.execute_config("runcode_main_login.cfg");}catch(const std::exception &){rejected=true;}check(rejected);
 }
 bool available=false;
 auto withShell=[&](const std::string &name){
  if(name!="interface_shell.cfg"||!available)return loader(name);
  std::string cfg=R"(
ConfigureInterface(){DefineControlType("ShellMainSmallButton","Button"){Size(100,30);Font("System");}}
CreateControl("Shell::Main","Window"){Size(640,480);
 CreateControl("Options","ShellMainSmallButton"){Pos(10,10);TranslateEvent("Button::Notify::Pressed","Options::Enter","|Shell::EventHandler");}
 CreateControl("Logout","ShellMainSmallButton"){Pos(10,60);OnEvent("Button::Notify::Pressed"){Op("gamegod.flow.state","=","Logout");SendNotifyEvent("|Shell::EventHandler","Main::Exit");}}
 CreateControl("Campaigns","ShellMainSmallButton"){Pos(10,110);OnEvent("Button::Notify::Pressed"){Cmd("sys.runcode mission");}}
})";
  return std::vector<std::uint8_t>(cfg.begin(),cfg.end());
 };
 {reign::MenuFrontend menu(withShell,root,640,480);
  bool missing=false;try{menu.login_selected_profile();}catch(const std::runtime_error &e){missing=std::string(e.what()).find("shell.zwp")!=std::string::npos;}check(missing&&menu.control_active("DlgLogin"));
  available=true;menu.login_selected_profile();check(menu.control_active("Shell::Main")&&!menu.control_active("DlgLogin"));
  auto frame=menu.draw([](const auto &){},[](const auto &,const auto &){},0,0);check(frame.geometry_batches>0);
  auto click=[&](const std::string &name){auto p=menu.control_center(name);menu.pointer_move(p[0],p[1]);menu.pointer_button(true,p[0],p[1]);menu.pointer_button(false,p[0],p[1]);};
  menu.take_action();click("Shell::Main.Campaigns");check(menu.take_action().empty()&&menu.control_active("Shell::Main"));
  click("Shell::Main.Options");check(menu.take_action()=="@options");
  menu.set_effect_volume(.37f);check(menu.effect_volume()==.37f);
  click("Shell::Main.Logout");check(menu.profile_name().empty()&&menu.control_active("DlgLogin")&&!menu.control_active("Shell::Main"));
  menu.login_selected_profile();check(menu.control_active("Shell::Main")&&menu.effect_volume()==.37f);menu.close();
 }
 std::cout<<"Persistent original menu/profile ownership, frame exception recovery and reopen: PASS ("<<checks<<" checks)\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
