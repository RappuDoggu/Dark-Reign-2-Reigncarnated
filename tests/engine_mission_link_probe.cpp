// Link-only check. This deliberately has no pretend engine bootstrap: running
// it cannot initialize Android graphics or instantiate a mission.
#include "defines.h"
#include <cstddef>
#include <cstdio>
#include "game.h"
#include "vid_native.h"
#include "audio_android.h"
namespace Sound { void Init(); void Done(); void Poll(); namespace Digital { void Claim(unsigned,unsigned); } }
extern "C" void RequireMissionEntryPoints() {
 Game::Init();
 Game::PreOpenMission();
 Game::OpenMission();
 Game::CloseMission();
 Game::PostCloseMission();
 Game::Done();
}
extern "C" void RequireNativeVideoBoundary() {
 NativeEngine::AttachVideo(nullptr, nullptr, nullptr, nullptr);
 NativeEngine::DetachVideo();
}
extern "C" void RequireNativeAudioBoundary() {
 NativeEngine::AndroidAudioOutput output(NativeEngine::GetDigitalMixer());
 output.Start();
 Sound::Init(); Sound::Digital::Claim(8,0); Sound::Poll(); Sound::Done();
 output.Stop();
}
#ifdef REIGN_LINK_FRAME_LOOP
namespace Game { namespace RC { void Init(); void Set(const char *); void Process(); void Done(); } }
extern "C" void RequireFrameLoop() {
 Game::RC::Init(); Game::RC::Set("Load"); Game::RC::Process(); Game::RC::Done();
}
#endif
int main() {
 std::fputs("Link check only; not a mission runner or Android app.\n",stderr);
 return 1;
}
