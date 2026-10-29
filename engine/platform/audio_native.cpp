#include "audio_native.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace NativeEngine {
std::shared_ptr<const AudioClip> DecodeWave(const void *input,std::size_t size){
 if(!input||size<12||size>64*1024*1024)throw std::invalid_argument("Invalid WAV size");
 const auto *b=static_cast<const std::uint8_t *>(input);
 auto u16=[&](std::size_t p){return unsigned(b[p])|(unsigned(b[p+1])<<8);};
 auto u32=[&](std::size_t p){return std::uint32_t(u16(p))|(std::uint32_t(u16(p+2))<<16);};
 if(std::memcmp(b,"RIFF",4)||std::memcmp(b+8,"WAVE",4)||u32(4)<4||std::uint64_t(u32(4))+8>size)throw std::invalid_argument("Invalid RIFF/WAVE bounds");
 const std::size_t end=std::size_t(u32(4))+8;std::size_t pcm=0,bytes=0;unsigned channels=0,rate=0,bits=0,align=0;bool format=false;
 for(std::size_t at=12;at<end;){
  if(end-at<8)throw std::invalid_argument("Truncated WAV chunk");
  const std::size_t n=u32(at+4),start=at+8;if(n>end-start)throw std::invalid_argument("WAV chunk outside RIFF");
  if(!std::memcmp(b+at,"fmt ",4)){
   if(format||n<16||u16(start)!=1)throw std::invalid_argument("Only PCM WAV is supported");
   channels=u16(start+2);rate=u32(start+4);align=u16(start+12);bits=u16(start+14);
   if((channels!=1&&channels!=2)||rate<8000||rate>96000||(bits!=8&&bits!=16)||align!=channels*(bits/8)||u32(start+8)!=rate*align)throw std::invalid_argument("Unsupported PCM WAV format");
   format=true;
  }else if(!std::memcmp(b+at,"data",4)){
   if(pcm)throw std::invalid_argument("Multiple WAV data chunks");pcm=start;bytes=n;
  }
  const auto next=start+n+(n&1);if(next>end)throw std::invalid_argument("Missing WAV padding");at=next;
 }
 if(!format||!pcm||!bytes||bytes%align)throw std::invalid_argument("Missing or unaligned PCM data");
 auto clip=std::make_shared<AudioClip>();clip->rate=rate;clip->stereo.resize(bytes/align*2);
 for(std::size_t frame=0;frame<bytes/align;++frame)for(unsigned c=0;c<2;++c){const auto at=pcm+frame*align+std::min(c,channels-1)*(bits/8);float sample;
  if(bits==8)sample=(int(b[at])-128)/128.f;else {const unsigned raw=u16(at);sample=(raw>=32768?int(raw)-65536:int(raw))/32768.f;}
  clip->stereo[frame*2+c]=sample;
 }
 return clip;
}
AudioMixer::AudioMixer(unsigned n):voices(n){if(!n||n>256)throw std::invalid_argument("Invalid audio voice capacity");}
std::uint64_t AudioMixer::Play(std::shared_ptr<const AudioClip> clip,float gain,int pan,int loops,float priority,std::uint32_t owner,const void *effect,const void *record){
 if(!OutputRunning()||!clip||clip->rate<8000||clip->rate>96000||clip->stereo.empty()||clip->stereo.size()%2||loops<0||!std::isfinite(gain)||!std::isfinite(priority))return 0;
 std::lock_guard<std::mutex> lock(mutex);
 auto slot=std::find_if(voices.begin(),voices.end(),[](const Voice &v){return !v.id;});
 if(owner){for(auto &v:voices)if(v.id&&v.owner==owner){v.id=0;if(slot==voices.end())slot=&v-&voices[0]+voices.begin();}}
 if(slot==voices.end()){slot=std::min_element(voices.begin(),voices.end(),[](const auto&a,const auto&b){return a.priority<b.priority;});if(slot->priority>priority)return 0;}
 if(++serial==0)++serial;
 *slot=Voice{std::move(clip),serial,0,loops,std::clamp(gain,0.f,1.f),priority,std::clamp(pan,0,127),owner,effect,record};return serial;
}
void AudioMixer::Render(float *out,std::size_t frames,unsigned rate)noexcept{
 if(!out||frames>65536)return;std::fill(out,out+frames*2,0.f);if(rate<8000||rate>192000||!OutputRunning())return;
 std::lock_guard<std::mutex> lock(mutex);
 for(auto &v:voices)if(v.id){
  const auto count=v.clip->stereo.size()/2;
  if(v.denominator!=rate){v.position=v.position*rate/v.denominator;v.denominator=rate;}
  const std::uint64_t limit=std::uint64_t(count)*rate;
  const float left=v.volume*volume*(v.pan<=64?1.f:float(127-v.pan)/63.f),right=v.volume*volume*(v.pan>=64?1.f:float(v.pan)/64.f);
  for(std::size_t f=0;f<frames;++f){
   while(v.position>=limit){v.position-=limit;if(v.loops>0&&--v.loops==0){v.id=0;break;}}
   if(!v.id)break;
   const auto first=std::size_t(v.position/rate);const auto next=std::min(first+1,count-1);const float fraction=float(v.position%rate)/rate;
   for(unsigned c=0;c<2;++c){const float a=v.clip->stereo[first*2+c],b=v.clip->stereo[next*2+c];out[f*2+c]+=(a+(b-a)*fraction)*(c?right:left);}
   v.position+=v.clip->rate;
  }
  // Publish completion even when the last sample exactly fills this callback.
  while(v.id&&v.position>=limit){v.position-=limit;if(v.loops>0&&--v.loops==0)v.id=0;}
 }
 for(std::size_t i=0;i<frames*2;++i)out[i]=std::clamp(out[i],-1.f,1.f);
}
bool AudioMixer::Playing(std::uint64_t id){std::lock_guard<std::mutex> l(mutex);return id&&std::any_of(voices.begin(),voices.end(),[&](const Voice&v){return v.id==id;});}
void AudioMixer::Stop(std::uint64_t id){std::lock_guard<std::mutex> l(mutex);for(auto&v:voices)if(v.id==id)v.id=0;}
void AudioMixer::StopAll(){std::lock_guard<std::mutex> l(mutex);for(auto&v:voices)v.id=0;}
void AudioMixer::StopEffect(const void *p){std::lock_guard<std::mutex> l(mutex);for(auto&v:voices)if(v.effect==p)v.id=0;}
void AudioMixer::StopRecord(const void *p){std::lock_guard<std::mutex> l(mutex);for(auto&v:voices)if(v.record==p)v.id=0;}
void AudioMixer::StopOwner(std::uint32_t p){std::lock_guard<std::mutex> l(mutex);for(auto&v:voices)if(v.owner==p)v.id=0;}
bool AudioMixer::EffectPlaying(const void*p){std::lock_guard<std::mutex> l(mutex);return std::any_of(voices.begin(),voices.end(),[&](const Voice&v){return v.id&&v.effect==p;});}
bool AudioMixer::RecordPlaying(const void*p){std::lock_guard<std::mutex> l(mutex);return std::any_of(voices.begin(),voices.end(),[&](const Voice&v){return v.id&&v.record==p;});}
bool AudioMixer::UpdateEffect(const void*p,float gain,int pan,float priority){if(!std::isfinite(gain)||!std::isfinite(priority))return false;std::lock_guard<std::mutex> l(mutex);bool hit=false;for(auto&v:voices)if(v.id&&v.effect==p){v.volume=std::clamp(gain,0.f,1.f);v.pan=std::clamp(pan,0,127);v.priority=priority;hit=true;}return hit;}
void AudioMixer::SetVolume(float v){std::lock_guard<std::mutex> l(mutex);volume=std::isfinite(v)?std::clamp(v,0.f,1.f):0.f;}
float AudioMixer::Volume(){std::lock_guard<std::mutex> l(mutex);return volume;}
std::shared_ptr<AudioMixer> GetDigitalMixer(){static auto mixer=std::make_shared<AudioMixer>();return mixer;}
}
