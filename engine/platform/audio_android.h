#pragma once
#include "audio_native.h"
struct AAudioStreamStruct;
using AAudioStream=AAudioStreamStruct;
namespace NativeEngine {
// Owns a real Android output stream. Start only publishes running state after
// AAudio accepts startup; destructor stops callbacks before releasing the mixer.
class AndroidAudioOutput {
 std::shared_ptr<AudioMixer> mixer;
 AAudioStream *stream=nullptr;
 std::atomic<int> lastError{0};
 std::atomic<unsigned> errors{0};
 unsigned actualRate=0,attempts=0;
public:
 explicit AndroidAudioOutput(std::shared_ptr<AudioMixer>);
 ~AndroidAudioOutput();
 AndroidAudioOutput(const AndroidAudioOutput &)=delete;
 AndroidAudioOutput &operator=(const AndroidAudioOutput &)=delete;
 bool Start();
 void Stop() noexcept;
 std::string Diagnostics()const;
};
}
