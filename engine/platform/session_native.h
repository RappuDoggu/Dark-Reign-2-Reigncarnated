#pragma once
#include <string>
namespace NativeEngine {
struct OfflineSessionInfo {std::string mission,player;};
// This native target supports one local player. No network transport is exposed.
// Begin/reset clears previous local session data and CRC before a fresh mission.
void BeginOfflineSession(std::string mission,std::string player);
void EndOfflineSession();
const OfflineSessionInfo *CurrentOfflineSession();
}
