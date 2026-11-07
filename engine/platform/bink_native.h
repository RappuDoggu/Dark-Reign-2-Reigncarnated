#pragma once
#include <cstddef>
#include <cstdint>
struct NativeBinkState;
// Only fields consumed by the original source are public. No binary Bink SDK ABI
// is promised: this is a native source-level adapter to the existing decoder.
struct BINK { std::uint32_t Width,Height,Frames,FrameNum; NativeBinkState *native; };
using HBINK=BINK *;
constexpr std::uint32_t BINKSURFACE32=3,BINKSURFACE555=9,BINKSURFACE565=10,BINKSURFACE655=11,BINKSURFACE664=12;
constexpr std::uint32_t BINKCOPYALL=0x80000000u,BINKNOTHREADEDIO=0x08000000u,BINKGOTOQUICK=1;
namespace NativeEngine {
using MovieAudioSink=void (*)(const std::int16_t *stereo,std::size_t samples,unsigned rate,void *context);
// Engine-thread configuration; sink consumes/copies PCM before returning.
void SetMovieAudioSink(MovieAudioSink,void *context);
HBINK OpenMovie(const void *bytes,std::size_t size);
}
const char *BinkGetError();
void BinkClose(HBINK);
void BinkDoFrame(HBINK);
void BinkNextFrame(HBINK);
int BinkWait(HBINK);
int BinkPause(HBINK,int pause);
void BinkGoto(HBINK,std::uint32_t frame,std::uint32_t flags);
void BinkCopyToBuffer(HBINK,void *destination,std::int32_t pitch,std::uint32_t height,std::int32_t x,std::int32_t y,std::uint32_t flags);
