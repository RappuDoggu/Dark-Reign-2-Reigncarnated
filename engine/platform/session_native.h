#pragma once
#include <string>
namespace NativeEngine {
struct OfflineSessionInfo {std::string mission,player;bool campaign=false;unsigned opponents=0,seed=1;std::string difficulty;};
// This native target supports one local player. No network transport is exposed.
// Player names use the original 16-byte identifier (15 bytes plus terminator).
// Begin/reset clears previous local session data and CRC before a fresh mission.
void BeginOfflineSession(std::string mission,std::string player,bool campaign=false,unsigned opponents=0,unsigned seed=1,std::string difficulty="");
void EndOfflineSession();
const OfflineSessionInfo *CurrentOfflineSession();
}
