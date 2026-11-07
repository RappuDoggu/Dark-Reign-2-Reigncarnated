#include "mission_prelude.h"
#include "missions.h"
#include "campaigns.h"
#include "difficulty.h"
#include "main_runtime.h"
#include <filesystem>
#include <iostream>
// This harness owns a genuine original RunCodes controller, but intentionally
// does not register/execute a fake game handler. Only its pending request is tested.
namespace Main { RunCodes runCodes("Game"); }
void check(bool ok,const char *message) { if(!ok) throw std::runtime_error(message); }
int main(int argc,char **argv) {
  bool files=false,campaigns=false,missions=false,difficulty=false;
  auto previous=std::filesystem::current_path();
  int result=0;
  try {
    if(argc!=2) throw std::runtime_error("Usage: engine_mission_start_test owned-installation");
    std::filesystem::current_path(std::filesystem::canonical(argv[1]));
    FileSys::Init(); files=true;
    check(FileSys::AddSrcPack("base","packs\\base.zwp"),"Base resource mount");
    check(FileSys::SetActiveStream("base"),"Base stream activation");
    Campaigns::Init(); campaigns=true;
    Missions::Init(); missions=true;
    Difficulty::Init(); difficulty=true;
    auto group=Missions::FindGroup("missions\\multiplayer");
    check(group && group->InstantAction(),"Original Instant Action group");
    auto selected=group->FindMission("2crazed");
    check(selected,"Original mission discovery");
    Missions::SetSelected(selected);
    check(Missions::GetSelected()==selected,"Selection retained");
    // These are the same mission-manager calls used by Game::PreOpenMission.
    // Game::PreOpenMission itself is not linked or executed by this target.
    Missions::SetActive(Missions::GetSelected());
    Missions::OpenActiveStream();
    check(FileSys::SetActiveStream("mission"),"Original mission stream activation");
    {
      PTree config; check(config.AddFile("game.cfg"),"Original active mission configuration");
      auto world=config.GetGlobalScope()->GetFunction("WorldInfo");
      check(world->GetFunction("CellMapX")->NextArgInteger()==100,"Expected mission width");
      check(world->GetFunction("CellMapZ")->NextArgInteger()==100,"Expected mission height");
    }
    // Registration allows testing Set's request only; never call Process here.
    Main::runCodes.Register("Mission",nullptr);
    Missions::LaunchMission(FALSE);
    check(Main::runCodes.GetNextCrc()==Crc::CalcStr("Mission"),"Original launch queues Mission runcode");
    Missions::CloseActiveStream();
    check(FileSys::SetActiveStream("base"),"Restore base stream");
    std::cout<<"PASS original Missions: discover -> select 2crazed -> activate -> open game.cfg -> queue Mission -> unload\n"
      <<"NOT EXECUTED: Game::PreOpenMission, player/team creation, simulation, renderer, touch controls\n";
  } catch(const std::exception &e) { std::cerr<<e.what()<<'\n';result=1; }
  Main::runCodes.Cleanup();
  if(difficulty) Difficulty::Done();
  if(campaigns) Campaigns::Done();
  if(missions || Missions::Initialized()) Missions::Done();
  if(files) FileSys::Done();
  std::filesystem::current_path(previous);
  return result;
}
