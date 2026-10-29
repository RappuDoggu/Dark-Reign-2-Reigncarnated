#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>
namespace NativeEngine {
struct MusicTrack { std::uint32_t number; std::filesystem::path path; };
// Engine-thread interface to the application's audio player. Start must report
// actual playback acceptance; no backend means no successful Play result.
class MusicPlayer {
public:
 virtual ~MusicPlayer() = default;
 virtual bool Start(const std::filesystem::path &) = 0;
 virtual void Stop() noexcept = 0;
 virtual bool Playing() const noexcept = 0;
 virtual void SetVolume(float) noexcept = 0;
};
// Retains the player. Invalid/duplicate/missing tracks leave the old state intact.
void InstallMusicPlayer(std::shared_ptr<MusicPlayer>, std::vector<MusicTrack>);
void RemoveMusicPlayer() noexcept;
}
// Fixed-width declarations matching the original sound.h Redbook ABI.
namespace Sound { namespace Redbook {
void Stop();
std::uint32_t Play(std::uint32_t);
std::uint32_t TrackCount();
std::uint32_t CurrentTrack();
void SetMinTrack(std::uint32_t);
void SetVolume(float);
float Volume();
void Poll();
int Claimed();
int Claim();
int Release();
int GetEnabled();
void SetEnabled(int);
} }
