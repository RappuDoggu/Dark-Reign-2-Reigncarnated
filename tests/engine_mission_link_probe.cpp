// Link-only check. This deliberately has no pretend engine bootstrap: running
// it cannot initialize Android graphics or instantiate a mission.
#include "defines.h"
#include <cstddef>
#include <cstdio>
#include "game.h"
extern "C" void RequireMissionEntryPoints() {
 Game::Init();
 Game::PreOpenMission();
 Game::OpenMission();
 Game::CloseMission();
 Game::PostCloseMission();
 Game::Done();
}
int main() {
 std::fputs("Link check only; not a mission runner or Android app.\n",stderr);
 return 1;
}
