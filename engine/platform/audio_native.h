#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>
namespace NativeEngine {
struct AudioClip { unsigned rate=0; std::vector<float> stereo; };
std::shared_ptr<const AudioClip> DecodeWave(const void *,std::size_t);
class AudioMixer {
 struct Voice { std::shared_ptr<const AudioClip> clip; std::uint64_t id=0; std::uint64_t position=0; int loops=1; float volume=1,priority=0; int pan=64; std::uint32_t owner=0; const void *effect=nullptr,*record=nullptr; unsigned denominator=48000; };
 std::mutex mutex;
 std::vector<Voice> voices;
 std::uint64_t serial=0;
 float volume=1;
 std::atomic<bool> running{false};
public:
 explicit AudioMixer(unsigned capacity=64);
 void SetOutputRunning(bool value)noexcept {running.store(value);}
 bool OutputRunning()const noexcept {return running.load();}
 std::uint64_t Play(std::shared_ptr<const AudioClip>,float volume,int pan,int loops,float priority=0,std::uint32_t owner=0,const void *effect=nullptr,const void *record=nullptr);
 void Render(float *stereo,std::size_t frames,unsigned rate=48000)noexcept;
 bool Playing(std::uint64_t);
 void Stop(std::uint64_t);
 void StopAll();
 void StopEffect(const void *);
 void StopRecord(const void *);
 void StopOwner(std::uint32_t);
 bool EffectPlaying(const void *);
 bool RecordPlaying(const void *);
 bool UpdateEffect(const void *,float,int,float);
 void SetVolume(float);
 float Volume();
};
std::shared_ptr<AudioMixer> GetDigitalMixer();
}
