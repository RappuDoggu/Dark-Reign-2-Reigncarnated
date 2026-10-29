#include "session_native.h"
#include "multiplayer.h"
#include "multiplayer_data.h"
#include <optional>
#include <stdexcept>
namespace {std::optional<NativeEngine::OfflineSessionInfo> offlineSession;}
namespace NativeEngine {
void BeginOfflineSession(std::string mission,std::string player){
 if(mission.empty()||mission.size()>255||player.empty()||player.size()>63)throw std::invalid_argument("Invalid local mission/player identity");
 OfflineSessionInfo next{std::move(mission),std::move(player)};
 MultiPlayer::Data::LocalFlush();MultiPlayer::SetDataCrc(0);offlineSession=std::move(next);
}
void EndOfflineSession(){MultiPlayer::Data::LocalFlush();MultiPlayer::SetDataCrc(0);offlineSession.reset();}
const OfflineSessionInfo *CurrentOfflineSession(){return offlineSession?&*offlineSession:nullptr;}
}
namespace MultiPlayer {
Bool IsHost(){return NativeEngine::CurrentOfflineSession()?TRUE:FALSE;}
void Load(FScope *scope){
 if(!scope)throw std::invalid_argument("Missing session save scope");
 if(!NativeEngine::CurrentOfflineSession())throw std::logic_error("Select a local mission before loading session state");
 // A plain local game has no network database. Restoring a multiplayer save
 // requires host/player reconstruction, so reject it before modifying state.
 if(scope->NextFunction())throw std::runtime_error("Multiplayer save-session restoration is not supported by the local mission target");
 Data::LocalFlush();
}
namespace Data {Bool Online(){return FALSE;} }
}
