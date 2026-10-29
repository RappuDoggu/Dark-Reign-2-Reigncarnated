#include "session_native.h"
#include "multiplayer.h"
#include "multiplayer_data.h"
#include <optional>
#include <map>
#include <memory>
#include "multiplayer_network.h"
#include <stdexcept>
namespace {
std::optional<NativeEngine::OfflineSessionInfo> offlineSession;
NBinTree<MultiPlayer::Network::Player,CRC> localPlayers(&MultiPlayer::Network::Player::node);
bool localReady=false;
std::map<U32,MultiPlayer::Data::Handler> localHandlers;
void RequireSession(){if(!offlineSession)throw std::logic_error("No active local session");}
}
namespace NativeEngine {
void BeginOfflineSession(std::string mission,std::string player){
 if(mission.empty()||mission.size()>255||player.empty()||player.size()>15||mission.find('\0')!=std::string::npos||player.find('\0')!=std::string::npos)throw std::invalid_argument("Invalid local mission/player identity");
 auto nextPlayer=std::make_unique<MultiPlayer::Network::Player>(MultiPlayer::UserName(player.c_str()));
 OfflineSessionInfo next{std::move(mission),std::move(player)};
 MultiPlayer::Data::LocalFlush();MultiPlayer::SetDataCrc(0);offlineSession=std::move(next);
 localPlayers.DisposeAll();localPlayers.Add(nextPlayer->GetId(),nextPlayer.get());nextPlayer.release();localReady=false;
}
void EndOfflineSession(){MultiPlayer::Data::LocalFlush();MultiPlayer::SetDataCrc(0);offlineSession.reset();localPlayers.DisposeAll();localReady=false;}
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

namespace MultiPlayer {
void ClearReady(){localReady=false;}
void SetReady(){RequireSession();localReady=true;}
Bool IsReady(){RequireSession();const bool ready=localReady;localReady=false;return ready?TRUE:FALSE;}
void Process(){RequireSession();} // No asynchronous transport in this build; sends dispatch locally.
void Save(FScope *){throw std::runtime_error("Local session save restoration is not implemented");}
namespace Network {const NBinTree<Player,CRC> &GetPlayers(){return localPlayers;}}
namespace Data {
U32 GetLag(){return 0;} // No network sync queue in the offline target.
Bool ProcessSyncData(U32 &,U32 &,U32 &){return FALSE;} // No packet; output references remain untouched.
void RegisterHandler(U32 key,Handler handler){if(!handler||!localHandlers.emplace(key,handler).second)throw std::logic_error("Invalid or duplicate local data handler");}
void UnRegisterHandler(U32 key){if(!localHandlers.erase(key))throw std::logic_error("Missing local data handler");}
void Send(CRC key,U32 size,const U8 *data,Bool){
 RequireSession();if(size&&!data)throw std::invalid_argument("Missing local message data");
 const auto found=localHandlers.find(key);
 if(found==localHandlers.end())throw std::runtime_error("No handler for local message");
 const auto handler=found->second;handler(MultiPlayer::UserName(offlineSession->player.c_str()).crc,key,size,data);
}
}
}
#include "session_game_native.inc"
namespace MultiPlayer {namespace Network {
Bool HaveCurrentPlayer(){return NativeEngine::CurrentOfflineSession()?TRUE:FALSE;}
const Player &GetCurrentPlayer(){RequireSession();NBinTree<Player,CRC>::Iterator player(&localPlayers);if(!*player)throw std::logic_error("Local player registry is empty");return **player;}
}}
