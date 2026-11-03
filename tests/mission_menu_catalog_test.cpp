#include "reign/mission_menu_catalog.hpp"
#include <fstream>
#include <chrono>
#include <iostream>
#include <stdexcept>
int main(){namespace fs=std::filesystem;auto root=fs::temp_directory_path()/("reign-menu-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));struct Cleanup{fs::path p;~Cleanup(){fs::remove_all(p);}} cleanup{root};
 try{auto group=root/"missions/test";fs::create_directories(group);std::ofstream(group/"alpha.zwp")<<"fixture";std::ofstream(group/"beta.zwp")<<"fixture";
 std::ofstream(group/"group.cfg")<<"InstantAction(1); Campaign() { Missions() { Add(\"beta\"); Add(\"alpha\"); Add(\"missing\"); } }";
 auto ia=reign::menu_missions(root,false),campaign=reign::menu_missions(root,true);
 if(ia.size()!=2||ia[0].name!="alpha"||ia[1].name!="beta"||campaign.size()!=1||campaign[0].name!="beta"||campaign[0].group!="missions/test")throw std::runtime_error("Map enumeration/campaign order failed");
 std::ofstream(root/"progress.cfg")<<"MissionProgress() { Mission(\"missions\\\\test\",\"beta\") { Won(1); Completed(0); } }";
 campaign=reign::menu_missions(root,true,root/"progress.cfg");if(campaign.size()!=2||campaign[1].name!="alpha")throw std::runtime_error("Completed mission did not unlock the next original mission");
 std::ofstream(group/"group.cfg")<<"System(1); InstantAction(1);";if(!reign::menu_missions(root,false).empty())throw std::runtime_error("System maps exposed");
 std::ofstream(group/"group.cfg")<<"Campaign() { Missions() { Add(\"alpha\"); Add(\"alpha\"); } }";bool rejected=false;try{reign::menu_missions(root,true);}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("Duplicate campaign entry accepted");
 std::cout<<"Imported menu catalog: IA filtering, campaign order, missing map omission, system filtering, duplicates PASS\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
