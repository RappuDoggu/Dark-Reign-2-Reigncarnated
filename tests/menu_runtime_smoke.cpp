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
CreateControl("DlgLogin","Game::Login"){Size(300,250);CreateControl("CreatePilot","Button"){Pos(20,200);Size(80,30);OnEvent("Button::Notify::Pressed"){Op("$<.newUser","=","AB");SendNotifyEvent("|DlgLogin","Game::Login::Message::Create");}}CreateControl("UserList","Listbox"){Size(200,150);UseVar("$<.user");}}
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
 std::cout<<"Persistent original menu/profile ownership, frame exception recovery and reopen: PASS ("<<checks<<" checks)\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
