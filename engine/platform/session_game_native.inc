// Single-player setup uses the original Team/Player machinery and mission data.
#include "game.h"
#include "gameconstants.h"
#include "unitobj.h"
#include "team.h"
#include "player.h"
#include "random.h"
namespace MultiPlayer {
void SetupPreMission(){
 if(!NativeEngine::CurrentOfflineSession())throw std::logic_error("No local mission selected");
 Random::sync.SetSeed(1);SetDataCrc(Random::sync.Raw());
}
void SetupMission(){
 const auto *session=NativeEngine::CurrentOfflineSession();
 if(!session||!Game::MissionPreLoaded())throw std::logic_error("Local mission must be preloaded before player setup");
 if(::Player::GetCurrentPlayer())throw std::logic_error("Previous mission player was not cleaned up");
 ::Team *selected=nullptr;
 for(U32 id=0;id<Game::MAX_TEAMS;++id){auto *team=::Team::Id2Team(id);if(team&&team->IsAvailablePlay()){selected=team;break;}}
 if(!selected)throw std::runtime_error("Mission has no playable starting team");
 auto *player=new ::Player(session->player.c_str());
 player->SetTeam(selected);::Player::SetCurrentPlayer(player);
 // Preserve the mission's starting side, resources and objects. No AI players
 // are created; the original purge removes unoccupied playable teams.
 ::Team::Purge();
}
void SetupPostMission(){
 if(!NativeEngine::CurrentOfflineSession()||!Game::MissionLoaded()||!::Player::GetCurrentPlayer()||!::Player::GetCurrentPlayer()->GetTeam())throw std::logic_error("Local mission/player setup is incomplete");
}
}
