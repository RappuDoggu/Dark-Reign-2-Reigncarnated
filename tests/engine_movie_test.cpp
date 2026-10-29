#include "bink_native.h"
#include "reign/bink_movie.hpp"
#include <fstream>
#include <iterator>
#include <iostream>
#include <vector>
#include <memory>
#include <stdexcept>
#include <algorithm>
void check(bool ok){if(!ok)throw std::runtime_error("Native movie adapter invariant");}
struct Audio {std::vector<std::int16_t> samples;unsigned rate=0;};
void Receive(const std::int16_t *pcm,std::size_t n,unsigned rate,void *context){auto &a=*static_cast<Audio *>(context);a.samples.insert(a.samples.end(),pcm,pcm+n);a.rate=rate;}
int main(int argc,char **argv){try{
 if(argc!=2)throw std::invalid_argument("Pass an owned Bink movie");
 std::ifstream file(argv[1],std::ios::binary);if(!file)throw std::runtime_error("Movie file unavailable");
 std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)),{});
 reign::BinkMovie reference(bytes);Audio audio;
 NativeEngine::SetMovieAudioSink(nullptr,nullptr);
 if(reference.sample_rate()){check(!NativeEngine::OpenMovie(bytes.data(),bytes.size()));check(std::string(BinkGetError()).find("audio sink")!=std::string::npos);}
 NativeEngine::SetMovieAudioSink(Receive,&audio);
 std::unique_ptr<BINK,decltype(&BinkClose)> movie(NativeEngine::OpenMovie(bytes.data(),bytes.size()),BinkClose);
 if(!movie)throw std::runtime_error(BinkGetError());auto *b=movie.get();
 check(b->Width==reference.width()&&b->Height==reference.height()&&b->FrameNum==1);
 check(BinkPause(b,1)==0&&BinkWait(b));check(BinkPause(b,1)==1);check(BinkPause(b,0)==1);
 const unsigned pitch=(b->Width+2)*4,height=b->Height+2;std::vector<std::uint8_t> target(std::size_t(pitch)*height,0xA5);
 bool rejected=false;try{BinkCopyToBuffer(b,target.data(),pitch,height,1,1,BINKSURFACE32);}catch(const std::logic_error &){rejected=true;}check(rejected);
 std::vector<std::int16_t> expectedAudio;
 auto u32=[&](unsigned n){return std::uint32_t(bytes[n])|(std::uint32_t(bytes[n+1])<<8)|(std::uint32_t(bytes[n+2])<<16)|(std::uint32_t(bytes[n+3])<<24);};
 const double fps=double(u32(28))/u32(32);
 for(unsigned frame=0;frame<b->Frames;++frame){
  check(b->FrameNum==frame+1);BinkDoFrame(b);const auto n=audio.samples.size();BinkDoFrame(b);check(audio.samples.size()==n);
  check(reference.advance((double(frame)+0.25)/fps));auto pcm=reference.drain_audio();expectedAudio.insert(expectedAudio.end(),pcm.begin(),pcm.end());
  BinkCopyToBuffer(b,target.data(),pitch,height,1,1,BINKSURFACE32|BINKCOPYALL|BINKNOTHREADEDIO);
  const auto &rgba=reference.rgba();
  for(unsigned y=0;y<b->Height;++y)for(unsigned x=0;x<b->Width;++x){const auto at=(std::size_t(y)*b->Width+x)*4;const auto to=(std::size_t(y)+1)*pitch+(x+1)*4;check(target[to]==rgba[at+2]&&target[to+1]==rgba[at+1]&&target[to+2]==rgba[at]&&target[to+3]==255);}
  check(target.front()==0xA5&&target.back()==0xA5);
  if(frame==std::min(20u,b->Frames-1)){
   for(auto format:{BINKSURFACE555,BINKSURFACE565,BINKSURFACE655,BINKSURFACE664}){
    std::vector<std::uint8_t> packed(std::size_t(b->Width)*b->Height*2);BinkCopyToBuffer(b,packed.data(),b->Width*2,b->Height,0,0,format);
    unsigned r=format==BINKSURFACE655||format==BINKSURFACE664?6:5,g=format==BINKSURFACE565||format==BINKSURFACE664?6:5,blue=format==BINKSURFACE664?4:5;
    for(std::size_t pixel=0;pixel<packed.size()/2;++pixel){unsigned expected=((rgba[pixel*4]>>(8-r))<<(g+blue))|((rgba[pixel*4+1]>>(8-g))<<blue)|(rgba[pixel*4+2]>>(8-blue));check((unsigned(packed[pixel*2])|(unsigned(packed[pixel*2+1])<<8))==expected);}
   }
  }
  BinkNextFrame(b);
 }
 check(b->FrameNum==1&&audio.samples==expectedAudio);if(reference.sample_rate())check(!audio.samples.empty()&&audio.rate==reference.sample_rate());
 BinkDoFrame(b);BinkCopyToBuffer(b,target.data(),pitch,height,1,1,BINKSURFACE32);
 rejected=false;try{BinkCopyToBuffer(b,target.data(),pitch,height,-1,0,BINKSURFACE32);}catch(const std::out_of_range &){rejected=true;}check(rejected);
 rejected=false;try{BinkGoto(b,b->Frames+1,0);}catch(const std::out_of_range &){rejected=true;}check(rejected&&b->FrameNum==1);
 BinkGoto(b,std::min(20u,b->Frames),BINKGOTOQUICK);BinkDoFrame(b);
 BinkGoto(b,0,0);check(b->FrameNum==1);BinkDoFrame(b);
 check(!NativeEngine::OpenMovie(bytes.data(),12));
 std::cout<<"Native movie adapter: "<<b->Frames<<" frames matched decoder pixels and "<<expectedAudio.size()<<" PCM samples; packing, bounds, pause, seek and loop passed\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
