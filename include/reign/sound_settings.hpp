#pragma once
#include <functional>
namespace reign {
// Explicit silent startup mode. Stores volume preferences, exposes no hardware
// providers and never reports a successful audio/CD claim or playback.
struct DisabledAudioSettings {
 float digital_volume=1,reserved_volume=1,redbook_volume=1;
 void run(const std::function<void()> &);
 static void register_class();
};
namespace disabled_audio_detail {
const char *LastError();unsigned RandomTrack(unsigned);
namespace Digital {
const char *GetServiceName2D();const char *GetServiceName3D();
bool Claimed3D();unsigned GetServiceIndex3D();
void SetProviderPreference(unsigned);bool Change3DProvider(bool claim=true);
bool Next3DProvider(const char *&,int &);
float Volume();void SetVolume(float);
namespace Reserved {float Volume();void SetVolume(float);}
}
namespace Redbook {
float Volume();void SetVolume(float);void SetEnabled(bool);bool GetEnabled();
bool Claimed();bool Claim();void Release();unsigned TrackCount();void Play(unsigned);
}
}
}
