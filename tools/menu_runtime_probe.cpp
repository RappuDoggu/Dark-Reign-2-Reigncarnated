#include "reign/menu_frontend.hpp"
#include "reign/menu_audio.hpp"
#include "reign/archive.hpp"
#include "reign/engine_file.hpp"
#include "reign/platform.hpp"
#include <iostream>
int main(int argc,char **argv){try{
 if(argc!=2&&argc!=3)throw std::invalid_argument("Usage: menu_runtime_probe base.zwp [shell.zwp]");
 reign::Archive archive(argv[1]);std::unique_ptr<reign::Archive> shell;if(argc==3)shell=std::make_unique<reign::Archive>(argv[2]);auto loader=[&](const std::string &name){auto pos=name.find_last_of("/\\");auto key=pos==std::string::npos?name:name.substr(pos+1);if(shell&&shell->find(key))return shell->read(key);if(!archive.find(key))throw FileSys::FileNotFound(key);return archive.read(key);};
 auto path=std::filesystem::temp_directory_path()/("reign-runtime-"+std::to_string(reign::monotonic_ns()));
 struct Cleanup{std::filesystem::path path;~Cleanup(){std::filesystem::remove_all(path);}} cleanup{path};
 for(unsigned pass=0;pass<2;++pass){
  unsigned sounds=0;reign::MenuAudio audio(loader,path/"audio");reign::MenuFrontend menu(loader,path,640,480,[&](const std::string &name){auto play=audio.load(name);return std::function<void(float)>([&,play](float volume){++sounds;play(volume);});});
  for(unsigned frame=0;frame<3;++frame){auto report=menu.draw([](const auto &){},[](const auto &,const auto &){},frame*16,16);if(!report.login_active||!report.backdrop_active||!report.geometry_batches||!report.font_batches)throw std::runtime_error("Incomplete original Login frame");std::cout<<"Session "<<pass+1<<" frame "<<frame+1<<": "<<report.geometry_batches<<" geometry, "<<report.font_batches<<" font batches\n";}
  if(!pass){auto click=[&](const std::string &name){auto p=menu.control_center(name);menu.pointer_move(p[0],p[1]);menu.pointer_button(true,p[0],p[1]);menu.pointer_button(false,p[0],p[1]);};
   click("DlgLogin.Create");if(!menu.control_active("DlgLoginCreate"))throw std::runtime_error("Original Create button did not open modal");
   click("DlgLoginCreate.Name");menu.text("Pointer Pilot");menu.key(0x1c,true);menu.key(0x1c,false);
   if(menu.profile_name()!="Pointer Pilot")throw std::runtime_error("Original edit text did not reach profile");
   if(menu.control_active("DlgLoginCreate"))throw std::runtime_error("Original Enter event did not close Create dialog");
   std::cout<<"Original pointer/modal/edit/Enter creation: PASS; sound requests recorded (not played): "<<sounds<<"\n";
  }
  if(shell){
   auto p=menu.control_center("DlgLogin.Login");menu.pointer_move(p[0],p[1]);menu.pointer_button(true,p[0],p[1]);menu.pointer_button(false,p[0],p[1]);
   if(!menu.control_active("Shell::Main")||menu.control_active("DlgLogin"))throw std::runtime_error("Login did not enter shell");
   auto frame=menu.draw([](const auto &){},[](const auto &,const auto &){},64,16);
   std::cout<<"Release main menu: "<<frame.geometry_batches<<" geometry, "<<frame.font_batches<<" font batches\n";
   if(!frame.geometry_batches||!frame.font_batches)throw std::runtime_error("Empty main menu");
   menu.set_effect_volume(0.65f);menu.take_action();
   p=menu.control_center("Shell::Main.Options");menu.pointer_move(p[0],p[1]);menu.pointer_button(true,p[0],p[1]);menu.pointer_button(false,p[0],p[1]);
   if(menu.take_action()!="@options")throw std::runtime_error("Options did not reach host");
   p=menu.control_center("Shell::Main.Logout");menu.pointer_move(p[0],p[1]);menu.pointer_button(true,p[0],p[1]);menu.pointer_button(false,p[0],p[1]);
   if(!menu.control_active("DlgLogin")||menu.control_active("Shell::Main")||!menu.profile_name().empty())throw std::runtime_error("Logout failed");
   menu.login_selected_profile();
   if(!menu.control_active("Shell::Main"))throw std::runtime_error("Second login failed");
  }
  auto audioEvents=audio.drain();if(audioEvents.empty())throw std::runtime_error("No original audio events");std::cout<<"Validated original WAV cache/events: "<<audioEvents.size()<<"\n";
  menu.close();
 }
 std::cout<<"Persistent original Login frame, profile, teardown and reopen: PASS\n";
}catch(const std::exception &e){std::cerr<<"MENU_RUNTIME_FAIL "<<e.what()<<'\n';return 1;}}
