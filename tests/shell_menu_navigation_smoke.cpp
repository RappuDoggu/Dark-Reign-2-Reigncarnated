#include "reign/shell_menu_navigation.hpp"
#include "reign/platform.hpp"
#include <cmath>
#include <iostream>
static std::string objective(const char *name,const char *bookmark,const char *delay="2"){
 return std::string("CreateObjectType(\"")+name+"\",\"Objective\"){ObjectiveObj(){Action(){Cineractive(){At(0){SetBookmark(){Name(\""+bookmark+"\");}}At("+delay+"){Action(){Cmd(\"iface.sendnotifyevent Shell::EventHandler CineractiveCompleted\");}}}}}}";
}
int main(int argc,char **argv){try{
 unsigned checks=0;auto check=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Shell navigation check "+std::to_string(checks));};
 auto parse=[](const std::string &cfg){return reign::parse_config([&](const std::string &){return std::vector<std::uint8_t>(cfg.begin(),cfg.end());},"fixture.cfg");};
 auto enter=objective("Cineractive::Multi::Enter","Multi::Enter"),exit=objective("Cineractive::Multi::Exit","Multi::Exit");
 reign::ShellMenuNavigation flow(parse(enter+exit));using Page=reign::ShellMenuNavigation::Page;using Event=reign::ShellMenuNavigation::Event;
 check(!flow.main_ready(false)&&flow.main_ready(true));
 bool rejected=false;try{flow.begin_instant(0,false);}catch(const std::logic_error &){rejected=true;}check(rejected&&flow.page()==Page::Main);
 check(flow.begin_instant(1,true)=="Multi::Enter");check(flow.page()==Page::EnteringInstant);
 check(flow.advance(2.99f)==Event::None);check(flow.advance(3)==Event::ShowInstant);check(flow.page()==Page::Instant);
 // Original entry bookmark lasts 5 seconds; controls are activated at 2.
 check(flow.advance(4)==Event::None&&!flow.main_ready(true));
 check(flow.return_main(4)=="Multi::Exit");check(flow.advance(5.99f)==Event::None);check(flow.advance(6)==Event::ShowMain);
 check(flow.main_ready(false)); // Original return camera can still be moving.
 check(flow.begin_instant(6.5f,false)=="Multi::Enter");check(flow.advance(8.5f)==Event::ShowInstant);flow.launch();check(flow.page()==Page::Launching);
 rejected=false;try{flow.launch();}catch(const std::logic_error &){rejected=true;}check(rejected);
 flow.reset();check(flow.advance(100)==Event::None&&flow.page()==Page::Main&&!flow.main_ready(false));
 flow.begin_instant(101,true);check(flow.return_main(101.5f)=="Multi::Exit");check(flow.return_main(102).empty());check(flow.advance(103.5f)==Event::ShowMain);
 reign::ShellMenuNavigation custom(parse(objective("Cineractive::Multi::Enter","ImportedCity","2.5")+exit));check(custom.begin_instant(0,true)=="ImportedCity");check(custom.advance(2)==Event::None&&custom.advance(2.5f)==Event::ShowInstant);
 for(auto cfg:{enter,enter+enter+exit,objective("Cineractive::Multi::Enter","Multi::Enter","0")+exit}){
  rejected=false;try{reign::ShellMenuNavigation bad(parse(cfg));}catch(const std::runtime_error &){rejected=true;}check(rejected);
 }
 auto unsupported=enter+exit;auto position=unsupported.find("iface.sendnotifyevent Shell::EventHandler CineractiveCompleted");unsupported.replace(position,std::string("iface.sendnotifyevent Shell::EventHandler CineractiveCompleted").size(),"sys.runcode quit");
 rejected=false;try{reign::ShellMenuNavigation bad(parse(unsupported));}catch(const std::runtime_error &e){rejected=std::string(e.what()).find("Unsupported shell navigation completion command")!=std::string::npos;}check(rejected);
 if(argc>1){auto path=std::filesystem::path(argv[1]);auto nodes=reign::parse_config([&](const std::string &n){return reign::read_asset(path.parent_path(),n);},path.filename().string());reign::ShellMenuNavigation real(nodes);check(real.begin_instant(0,true)=="Multi::Enter");check(real.advance(1.99f)==Event::None&&real.advance(2)==Event::ShowInstant);check(real.return_main(3)=="Multi::Exit");check(real.advance(5)==Event::ShowMain);}
 std::cout<<"Shell navigation: imported city bookmarks, objective-driven activation before camera completion, Back, reset, duplicate suppression and explicit errors PASS ("<<checks<<" checks)\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
