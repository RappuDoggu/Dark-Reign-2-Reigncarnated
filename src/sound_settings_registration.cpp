#include "icsoundconfig.h"
#include "reign/sound_settings.hpp"
namespace {IControl *factory(U32,IControl *parent,U32){return new ICSoundConfig(parent);}}
namespace reign {void DisabledAudioSettings::register_class(){IFace::RegisterControlClass("SoundConfig",factory);}}
