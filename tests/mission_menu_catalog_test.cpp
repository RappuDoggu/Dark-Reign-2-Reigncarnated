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
 auto all=reign::menu_campaign_testing(root);if(all.size()!=3||!all[0].first||all[1].first||all[2].available||all[0].mission.name!="beta")throw std::runtime_error("Unrestricted catalog omitted locked/missing entries or reordered the campaign opening");
  std::ofstream(group/"group.cfg")<<"System(1); InstantAction(1);";if(!reign::menu_missions(root,false).empty())throw std::runtime_error("System maps exposed");
 std::ofstream(group/"group.cfg")<<"Campaign() { Missions() { Add(\"alpha\"); Add(\"alpha\"); } }";bool rejected=false;try{reign::menu_missions(root,true);}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("Duplicate campaign entry accepted");rejected=false;try{reign::menu_campaign_testing(root);}catch(const std::runtime_error &){rejected=true;}if(!rejected)throw std::runtime_error("Duplicate testing catalog entry accepted");

 fs::remove_all(root/"missions");
 const std::vector<std::string> jda={"J01","J02","J03","J04","J05","J06_intro","J06","J07","J08","J09","J10"};
 const std::vector<std::string> sprawl={"S01_Intro","Transition into S01","S01","Transition into S02","S02","Transition into S03","S03","Transition into S04","S04","Transition into S05","S05","Transition into S06","S06","Transition into S07","S07","Transition into S08","S08","Transition into S09","S09","Outro"};
 for(const auto &g:{std::make_pair(std::string("jda"),jda),std::make_pair(std::string("sprawler"),sprawl)}){auto path=root/"missions"/g.first;fs::create_directories(path);std::ofstream cfg(path/"group.cfg");cfg<<"Campaign(){Missions(){";for(const auto &name:g.second){cfg<<"Add(\""<<name<<"\");";std::ofstream(path/(name+".zwp"))<<"Authored metadata fixture";}cfg<<"}}";}
 all=reign::menu_campaign_testing(root);unsigned scenes=0,starts=0;for(const auto &e:all){scenes+=e.cutscene;starts+=e.first;if(!e.available)throw std::runtime_error("Present campaign test asset not found");}
 if(all.size()!=31||scenes!=12||starts!=2||all[0].mission.name!="J01"||all[11].mission.name!="S01_Intro"||all.back().mission.name!="Outro")throw std::runtime_error("Full 31-entry campaign/cutscene catalog failed");
 fs::remove(root/"missions/sprawler/S01_Intro.zwp");all=reign::menu_campaign_testing(root);if(all.size()!=31||all[11].available||!all[11].first||all[12].first)throw std::runtime_error("Missing original opening was replaced with a later entry");
  std::cout<<"Imported menu catalog: IA filtering, campaign order, missing map omission, system filtering, duplicates, all 31 testing entries/12 cutscenes, explicit missing opening PASS\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
