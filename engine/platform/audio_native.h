#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>
#include <string>
namespace NativeEngine {
struct AudioClip { unsigned rate=0; std::vector<float> stereo; };
std::shared_ptr<const AudioClip> DecodeWave(const void *,std::size_t);
// Memory-only game samples: strict PCM WAV or bounded MPEG Layer III speech.
std::shared_ptr<const AudioClip> DecodeGameAudio(const void *,std::size_t);
class AudioMixer {
 struct Voice { std::shared_ptr<const AudioClip> clip; std::uint64_t id=0; std::uint64_t position=0; int loops=1; float volume=1,priority=0; int pan=64; std::uint32_t owner=0; const void *effect=nullptr,*record=nullptr; unsigned denominator=48000; };
 std::mutex mutex;
 std::vector<Voice> voices;
 // Stereo movie frames arrive on the engine thread and drain on the audio callback.
 static constexpr unsigned movieCapacity=96000;
 std::vector<std::int16_t> movieSamples;
 unsigned movieRead=0,movieCount=0,movieRate=0,moviePhase=0,movieOutputRate=0;
 std::uint64_t serial=0;
 float volume=1;
 std::atomic<bool> running{false};
 struct Counters {std::uint64_t requests=0,started=0,output_off=0,invalid=0,priority_rejected=0,released_effects=0,callbacks=0,frames=0,audible_callbacks=0;float peak=0;} counters;
public:
 explicit AudioMixer(unsigned capacity=64);
 void SetOutputRunning(bool value)noexcept {running.store(value);}
 bool OutputRunning()const noexcept {return running.load();}
 std::uint64_t Play(std::shared_ptr<const AudioClip>,float volume,int pan,int loops,float priority=0,std::uint32_t owner=0,const void *effect=nullptr,const void *record=nullptr);
 void QueueMovie(const std::int16_t *samples,std::size_t count,unsigned rate);
 void ClearMovie();
 void Render(float *stereo,std::size_t frames,unsigned rate=48000)noexcept;
 bool Playing(std::uint64_t);
 void Stop(std::uint64_t);
 void StopAll();
 void StopEffect(const void *);
 // A dead effect loses its association; already-started samples keep playing.
 void ReleaseEffect(const void *);
 void StopRecord(const void *);
 void StopOwner(std::uint32_t);
 bool EffectPlaying(const void *);
 bool RecordPlaying(const void *);
 bool UpdateEffect(const void *,float,int,float);
 void SetVolume(float);
 float Volume();
 std::string Diagnostics();
};
std::shared_ptr<AudioMixer> GetDigitalMixer();
}
