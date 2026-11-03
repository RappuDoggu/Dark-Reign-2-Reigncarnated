// Single-player setup uses the original Team/Player machinery and mission data.
#include "game.h"
#include "gameconstants.h"
#include "unitobj.h"
#include "team.h"
#include "player.h"
#include "random.h"
#include "tagobjdec.h"
#include "strategic.h"
#include "strategic_object.h"
#include "mods.h"
#include "difficulty.h"
namespace MultiPlayer {
void SetupPreMission(){
 if(!NativeEngine::CurrentOfflineSession())throw std::logic_error("No local mission selected");
 Random::sync.SetSeed(NativeEngine::CurrentOfflineSession()->seed);SetDataCrc(Random::sync.Raw());
}
void SetupMission(){
 const auto *session=NativeEngine::CurrentOfflineSession();
 if(!session||!Game::MissionPreLoaded())throw std::logic_error("Local mission must be preloaded before player setup");
 if(::Player::GetCurrentPlayer())throw std::logic_error("Previous mission player was not cleaned up");
 if(!session->difficulty.empty()){auto *setting=Difficulty::FindSetting(GameIdent(session->difficulty.c_str()));if(!setting)throw std::invalid_argument("Unknown imported difficulty setting");Difficulty::SetCurrentSetting(setting->GetName());}
 if(session->campaign)return; // Original Team::PostLoad creates campaign client and scripted AI.
 ::Team *selected=nullptr;
 for(U32 id=0;id<Game::MAX_TEAMS;++id){auto *team=::Team::Id2Team(id);if(team&&team->IsAvailablePlay()){selected=team;break;}}
 if(!selected)throw std::runtime_error("Mission has no playable starting team");
 auto *player=new ::Player(session->player.c_str());
 player->SetTeam(selected);::Player::SetCurrentPlayer(player);
 if(session->opponents){
  ::Team *enemy=nullptr;for(U32 id=0;id<Game::MAX_TEAMS;++id){auto *team=::Team::Id2Team(id);if(team&&team!=selected&&team->IsAvailablePlay()){enemy=team;break;}}
  if(!enemy)throw std::runtime_error("Selected map has no second playable starting team");
  auto *personality=Mods::GetRandomMod(Mods::Types::Personality);if(!personality)throw std::runtime_error("Import the AI personality mods before enabling an opponent");
  auto *ai=Strategic::Create(personality->GetName().crc);if(!ai)throw std::runtime_error("Original strategic AI could not be created");ai->SetTeam(enemy);enemy->SetPersonality(personality->GetName().str);enemy->SetDifficulty(Difficulty::GetCurrentSetting().GetValue());
  selected->SetRelation(enemy->GetId(),Relation(Relation::ENEMY));enemy->SetRelation(selected->GetId(),Relation(Relation::ENEMY));
 }
 // Remove only unoccupied playable teams, after retaining the selected AI team.
 ::Team::Purge();
}
void SetupPostMission(){
 if(!NativeEngine::CurrentOfflineSession()||!Game::MissionLoaded()||!::Player::GetCurrentPlayer()||!::Player::GetCurrentPlayer()->GetTeam())throw std::logic_error("Local mission/player setup is incomplete");
}
}
