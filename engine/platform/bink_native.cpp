#include "bink_native.h"
#include "reign/bink_movie.hpp"
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <algorithm>
using MovieClock=std::chrono::steady_clock;
struct NativeBinkState {
 std::vector<std::uint8_t> bytes;
 std::unique_ptr<reign::BinkMovie> movie;
 double fps;
 MovieClock::time_point epoch=MovieClock::now(),pausedAt=epoch;
 bool paused=false;
 NativeEngine::MovieAudioSink sink;
 void *context;
};
namespace {
thread_local std::string lastError;
NativeEngine::MovieAudioSink audioSink=nullptr;
void *audioContext=nullptr;
NativeBinkState &State(HBINK b){if(!b||!b->native)throw std::invalid_argument("Invalid native movie");return *b->native;}
double FrameTime(HBINK b){return (double(b->FrameNum-1)+0.25)/State(b).fps;}
void Rewind(HBINK b){auto &s=State(b);s.movie=std::make_unique<reign::BinkMovie>(s.bytes);}
}
namespace NativeEngine {
void SetMovieAudioSink(MovieAudioSink sink,void *context){audioSink=sink;audioContext=context;}
HBINK OpenMovie(const void *bytes,std::size_t size){
 try {
  if(!bytes||size<44||size>128u*1024u*1024u)throw std::invalid_argument("Invalid movie memory range");
  auto state=std::make_unique<NativeBinkState>();
  const auto *p=static_cast<const std::uint8_t *>(bytes);state->bytes.assign(p,p+size);
  auto u32=[&](unsigned at){return std::uint32_t(p[at])|(std::uint32_t(p[at+1])<<8)|(std::uint32_t(p[at+2])<<16)|(std::uint32_t(p[at+3])<<24);};
  state->movie=std::make_unique<reign::BinkMovie>(state->bytes);
  if(!u32(28)||!u32(32))throw std::invalid_argument("Invalid movie frame rate");
  state->fps=double(u32(28))/u32(32);
  state->sink=audioSink;state->context=audioContext;
  if(state->movie->sample_rate()&&!state->sink)throw std::logic_error("Movie audio sink must be configured before opening an audio movie");
  auto result=std::make_unique<BINK>();result->Width=state->movie->width();result->Height=state->movie->height();result->Frames=u32(8);result->FrameNum=1;
  result->native=state.release();lastError.clear();return result.release();
 }catch(const std::exception &e){lastError=e.what();return nullptr;}
}
}
const char *BinkGetError(){return lastError.c_str();}
void BinkClose(HBINK b){if(b){delete b->native;delete b;}}
void BinkDoFrame(HBINK b){
 auto &s=State(b);
 if(!s.movie->advance(FrameTime(b)))throw std::runtime_error("Native movie ended before its declared frame count");
 auto pcm=s.movie->drain_audio();
 if(!pcm.empty()){if(!s.sink)throw std::logic_error("Native movie audio has no consumer");s.sink(pcm.data(),pcm.size(),s.movie->sample_rate(),s.context);}
}
void BinkNextFrame(HBINK b){
 auto &s=State(b);
 if(b->FrameNum==b->Frames){b->FrameNum=1;Rewind(b);s.epoch=s.pausedAt=MovieClock::now();}
 else ++b->FrameNum;
}
int BinkWait(HBINK b){auto &s=State(b);return s.paused||std::chrono::duration<double>(MovieClock::now()-s.epoch).count()<double(b->FrameNum-1)/s.fps;}
int BinkPause(HBINK b,int pause){
 auto &s=State(b);const bool was=s.paused;const auto now=MovieClock::now();
 if(pause&&!s.paused){s.pausedAt=now;s.paused=true;}
 else if(!pause&&s.paused){s.epoch+=now-s.pausedAt;s.paused=false;}
 return was;
}
void BinkGoto(HBINK b,std::uint32_t frame,std::uint32_t flags){
 auto &s=State(b);if(flags!=BINKGOTOQUICK&&flags!=0)throw std::invalid_argument("Unsupported movie seek flags");
 frame=std::max(frame,1u);if(frame>b->Frames)throw std::out_of_range("Movie frame number");
 Rewind(b);b->FrameNum=frame;
 if(frame>1)s.movie->restore_position(FrameTime(b));
 const auto now=MovieClock::now();s.epoch=now-std::chrono::duration_cast<MovieClock::duration>(std::chrono::duration<double>(double(frame-1)/s.fps));s.pausedAt=now;
}
void BinkCopyToBuffer(HBINK b,void *destination,std::int32_t pitch,std::uint32_t height,std::int32_t x,std::int32_t y,std::uint32_t flags){
 auto &s=State(b);const auto format=flags&~(BINKCOPYALL|BINKNOTHREADEDIO);
 unsigned bytesPerPixel=2,redBits=5,greenBits=5,blueBits=5;
 switch(format){case BINKSURFACE32:bytesPerPixel=4;break;case BINKSURFACE555:break;case BINKSURFACE565:greenBits=6;break;case BINKSURFACE655:redBits=6;break;case BINKSURFACE664:redBits=6;greenBits=6;blueBits=4;break;default:throw std::invalid_argument("Unsupported movie pixel format");}
 if(!destination||pitch<=0||x<0||y<0||std::uint64_t(x+b->Width)*bytesPerPixel>std::uint32_t(pitch)||std::uint64_t(y)+b->Height>height)throw std::out_of_range("Movie destination rectangle");
 if(s.movie->frame_index()!=std::int64_t(b->FrameNum-1))throw std::logic_error("Movie frame must be decoded before copying");
 const auto &rgba=s.movie->rgba();auto *dst=static_cast<std::uint8_t *>(destination);
 for(unsigned row=0;row<b->Height;++row){
  auto *out=dst+(std::size_t(y)+row)*std::size_t(pitch)+std::size_t(x)*bytesPerPixel;
  const auto *in=rgba.data()+std::size_t(row)*b->Width*4;
  for(unsigned col=0;col<b->Width;++col,in+=4,out+=bytesPerPixel){
   if(bytesPerPixel==4){out[0]=in[2];out[1]=in[1];out[2]=in[0];out[3]=255;}
   else {const std::uint16_t pixel=(std::uint16_t(in[0]>>(8-redBits))<<(greenBits+blueBits))|(std::uint16_t(in[1]>>(8-greenBits))<<blueBits)|(in[2]>>(8-blueBits));out[0]=std::uint8_t(pixel);out[1]=std::uint8_t(pixel>>8);}
  }
 }
}
