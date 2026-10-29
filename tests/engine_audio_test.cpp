#include "audio_native.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#define check(v) do{if(!(v))throw std::runtime_error("Audio check failed at line "+std::to_string(__LINE__));}while(0)
std::vector<unsigned char> wave(){std::vector<unsigned char>b(52);auto w16=[&](int p,unsigned v){b[p]=v;b[p+1]=v>>8;};auto w32=[&](int p,unsigned v){w16(p,v);w16(p+2,v>>16);};std::memcpy(b.data(),"RIFF",4);w32(4,44);std::memcpy(b.data()+8,"WAVEfmt ",8);w32(16,16);w16(20,1);w16(22,1);w32(24,8000);w32(28,16000);w16(32,2);w16(34,16);std::memcpy(b.data()+36,"data",4);w32(40,8);w16(44,32767);w16(46,0);w16(48,32768);w16(50,16384);return b;}
int main(){try{
 using namespace NativeEngine;auto bytes=wave();auto clip=DecodeWave(bytes.data(),bytes.size());check(clip->rate==8000&&clip->stereo.size()==8&&clip->stereo[4]==-1);
 AudioMixer mixer(2);check(!mixer.Play(clip,1,64,1));mixer.SetOutputRunning(true);
 float output[48];auto id=mixer.Play(clip,1,64,1);check(id);mixer.Render(output,24);
 for(unsigned i=0;i<24;++i){const double at=double(i)/6;auto f=std::size_t(at);float fraction=float(at-f);float expected=clip->stereo[f*2]+(clip->stereo[std::min(f+1,std::size_t(3))*2]-clip->stereo[f*2])*fraction;check(std::abs(output[i*2]-expected)<0.00001f&&output[i*2]==output[i*2+1]);}
 check(!mixer.Playing(id));
 id=mixer.Play(clip,1,0,0,0,77,clip.get());mixer.Render(output,24);check(mixer.Playing(id)&&mixer.EffectPlaying(clip.get()));
 for(unsigned i=0;i<24;++i)check(output[i*2+1]==0);
 check(mixer.UpdateEffect(clip.get(),0.5f,127,0));mixer.Render(output,24);check(output[0]==0);
 mixer.StopOwner(77);check(!mixer.Playing(id));
 auto a=mixer.Play(clip,1,64,0,2),b=mixer.Play(clip,1,64,0,3);check(a&&b&&!mixer.Play(clip,1,64,1,1));
 auto c=mixer.Play(clip,1,64,1,4);check(c&&!mixer.Playing(a)&&mixer.Playing(b));
 mixer.StopAll();mixer.SetVolume(0.5f);id=mixer.Play(clip,1,64,1);mixer.Render(output,24);check(std::abs(output[0]-clip->stereo[0]*0.5f)<0.00001f);
 mixer.SetOutputRunning(false);mixer.Render(output,24);for(auto s:output)check(s==0);
 bool rejected=false;bytes[40]=250;try{DecodeWave(bytes.data(),bytes.size());}catch(const std::invalid_argument &){rejected=true;}check(rejected);
 std::cout<<"PCM bounds, conversion, resampling, panning, finite/infinite loops, priority, owner stop and output loss pass.\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
