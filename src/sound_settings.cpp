#include <stdexcept>
#include "reign/sound_settings.hpp"
#include <cmath>
namespace {
thread_local reign::DisabledAudioSettings *active=nullptr;
reign::DisabledAudioSettings &state(){if(!active)throw std::runtime_error("Audio settings backend not bound");return *active;}
void volume(float &value,float input){if(!std::isfinite(input)||input<0||input>1)throw std::invalid_argument("Audio volume outside [0,1]");value=input;}
}
namespace reign {
void DisabledAudioSettings::run(const std::function<void()> &call){if(!call)throw std::invalid_argument("Missing audio settings dispatch");auto *previous=active;active=this;struct Restore{DisabledAudioSettings *p;~Restore(){active=p;}} restore{previous};call();}
namespace disabled_audio_detail {
const char *LastError(){state();return "Audio disabled in this native build";}
unsigned RandomTrack(unsigned){state();throw std::runtime_error("CD audio is unavailable");}
namespace Digital {
const char *GetServiceName2D(){state();return "Disabled";}const char *GetServiceName3D(){state();return "Disabled";}
bool Claimed3D(){state();return false;}unsigned GetServiceIndex3D(){state();throw std::runtime_error("No audio provider claimed");}
void SetProviderPreference(unsigned){state();}bool Change3DProvider(bool){state();return false;}
bool Next3DProvider(const char *&name,int &first){state();name=nullptr;first=false;return false;}
float Volume(){return state().digital_volume;}void SetVolume(float value){volume(state().digital_volume,value);}
namespace Reserved {float Volume(){return state().reserved_volume;}void SetVolume(float value){volume(state().reserved_volume,value);}}
}
namespace Redbook {
float Volume(){return state().redbook_volume;}void SetVolume(float value){volume(state().redbook_volume,value);}
void SyncEnabled(){state();}
void SetEnabled(bool){state();}bool GetEnabled(){state();return false;}
bool Claimed(){state();return false;}bool Claim(){state();return false;}void Release(){state();}
unsigned TrackCount(){state();return 0;}void Play(unsigned){state();throw std::runtime_error("CD audio is unavailable");}
}
}
}
