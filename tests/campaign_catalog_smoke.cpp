#include "reign/mission_catalog.hpp"
#include "reign/mission_fs.hpp"
#include "reign/platform.hpp"
#include "missions.h"
#include "campaigns.h"
#include "difficulty.h"
#include "game_preview.h"
#include "reign/campaign_selection.hpp"
#include "bmp_fixture.hpp"
#include <fstream>
#include <iostream>
namespace fs=std::filesystem;
int main(){try{
 unsigned checks=0;auto check=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Catalog check "+std::to_string(checks));};
 auto rejects=[&](auto call){bool rejected=false;try{call();}catch(const std::exception &){rejected=true;}check(rejected);};
 auto path=fs::temp_directory_path()/("reign-catalog-"+std::to_string(reign::monotonic_ns()));if(!fs::create_directory(path))throw std::runtime_error("Temporary catalog exists");
 struct Cleanup{fs::path p;~Cleanup(){fs::remove_all(p);}} cleanup{path};
 fs::create_directories(path/"Missions"/"Sample"/"first");fs::create_directories(path/"local");
 std::ofstream(path/"Missions"/"Sample"/"second.zwp")<<"fixture catalog entry";
 std::ofstream(path/"Missions"/"Sample"/"group.cfg")<<"Description(\"Sample campaign\");Campaign(){Missions(){Add(\"first\");Add(\"second\");}}";
 std::ofstream(path/"Missions"/"Sample"/"first"/"game.cfg")<<"WorldInfo(){CellMapX(64);}DefaultRule(\"TestRule\");FixedRule(1);DefineTeams(){CreateTeam(\"Blue\",0){StartPoint(0.2,0.3);Side(\"FixtureSide\");}CreateTeam(\"Hidden\",1){AvailablePlay(0);}}";
 auto pixels=bmp_fixture(24);pixels[56]=255;{std::ofstream out(path/"Missions"/"Sample"/"first"/"terrain.bmp",std::ios::binary);out.write(reinterpret_cast<const char *>(pixels.data()),pixels.size());}
 std::string difficulty="CreateSetting(\"Normal\",1.0);CreateSetting(\"Hard\",1.4);DefaultSetting(\"Normal\");";
 auto loader=[&](const std::string &name){if(name!="difficulty.cfg")throw std::runtime_error("Unexpected catalog asset: "+name);return std::vector<std::uint8_t>(difficulty.begin(),difficulty.end());};
 {
  reign::MissionCatalog catalog(path,loader);rejects([&]{reign::MissionCatalog second(path,loader);});
  check(Missions::GetGroups().GetCount()==2);check(Campaigns::GetCampaigns().GetCount()==1);
  auto *first=Missions::FindMission("first","missions\\sample");auto *second=Missions::FindMission("second","missions\\sample");check(first&&second);
  check(Campaigns::NextMission(first)==second);check(Campaigns::NextMission(second)==nullptr);
  Missions::SetSelected(second);check(Missions::GetSelected()==second);
  check(Difficulty::GetDefaultSetting().GetValue()==1);Difficulty::SetCurrentSetting("Hard");check(Difficulty::GetCurrentSetting().GetValue()>1.39f);
  auto &progress=Campaigns::GetProgress();auto &record=progress.GetMission("missions\\Sample","first");record.SetDifficulty("Hard");record.RecordWin();check(record.MissionCompleted()&&record.MissionCompleted(TRUE));
  FScope saved(nullptr,"Progress");progress.SaveState(&saved);saved.InitIterators();Campaigns::Progress restored;struct ClearProgress{Campaigns::Progress &p;~ClearProgress(){p.Clear();}} clearProgress{restored};restored.LoadState(&saved);
  auto *restoredRecord=restored.FindMission("missions\\Sample","first");check(restoredRecord&&restoredRecord->MissionCompleted()&&std::string(restoredRecord->GetDifficulty().str)=="Hard");
  restored.ClearCompletion("missions\\Sample");check(!restoredRecord->MissionCompleted()&&restoredRecord->MissionCompleted(TRUE));
  reign::CampaignSelectionServices previewServices;unsigned requests=0;
  previewServices.open_mission=[&](const std::string &group,const std::string &name){return catalog.mission_loader(group,name);};
  previewServices.default_side=[]{return "FixtureSide";};previewServices.request_run_code=[&](std::uint32_t crc){check(crc==0xA293FAEB);++requests;};
  previewServices.run([&]{Game::Preview preview(first);check(preview.GetSize()==64&&preview.GetTeams()==1);check(preview.GetTerrainTexture()&&preview.GetTerrainTexture()->surface().pixels().pixels[0]==255);
   auto *team=preview.FindTeamId(0);check(team&&std::string(team->GetSide().str)=="FixtureSide");check(team->GetStartPoint().x>.19f&&team->GetStartPoint().y>.29f);check(preview.IsRuleSetFixed()&&std::string(preview.GetRuleSet().str)=="TestRule");
   preview.ReloadTextures();check(preview.GetTerrainTexture()->Width()==1);
   reign::preview_detail::request_run_code(0xA293FAEB);check(requests==1);
   rejects([&]{reign::preview_detail::load_texture("terrain.bmp");});
   previewServices.default_side={};rejects([&]{Game::Preview missingService(first);});
  });
  rejects([&]{Game::Preview unbound(first);});
  rejects([&]{catalog.mission_loader("../outside","first");});
  catalog.run([&]{check(reign::mission_fs::File::Exists("missions\\sample\\GROUP.CFG"));rejects([&]{reign::mission_fs::File::Exists("../outside");});});
  rejects([&]{reign::mission_fs::File::Exists("local");});
 }
 check(!Missions::Initialized());difficulty="CreateSetting(\"Invalid\",99.0);";rejects([&]{reign::MissionCatalog invalid(path,loader);});check(!Missions::Initialized());
 difficulty="CreateSetting(\"Normal\",1.0);";{reign::MissionCatalog recovered(path,loader);check(Missions::Initialized());}
 std::cout<<"Original mission catalog/progress/difficulty/preview metadata and texture/native paths: PASS ("<<checks<<" checks)\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
