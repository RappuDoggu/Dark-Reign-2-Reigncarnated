#include "audio_native.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#define check(v) do{if(!(v))throw std::runtime_error("Speech decode check line "+std::to_string(__LINE__));}while(0)
int main(int argc,char **argv){try{
 check(argc==3);
 for(int n=1;n<3;++n){std::ifstream file(argv[n],std::ios::binary);std::vector<char> bytes((std::istreambuf_iterator<char>(file)),{});check(bytes.size()>100);
  auto clip=NativeEngine::DecodeGameAudio(bytes.data(),bytes.size());check(clip->rate==(n==1?44100u:22050u));check(clip->stereo.size()/2>clip->rate*0.45&&clip->stereo.size()/2<clip->rate*0.6);
  float peak=0;for(float sample:clip->stereo){check(std::isfinite(sample));peak=std::max(peak,std::abs(sample));}check(peak>0.05f&&peak<0.2f);
  if(n==1)for(std::size_t i=0;i<clip->stereo.size();i+=2)check(clip->stereo[i]==clip->stereo[i+1]);
  NativeEngine::AudioMixer mixer;mixer.SetOutputRunning(true);auto id=mixer.Play(clip,1,64,1);check(id);float out[960];unsigned audible=0;
  for(unsigned i=0;i<65;++i){mixer.Render(out,480);if(std::any_of(out,out+960,[](float v){return std::abs(v)>0.01f;}))++audible;}check(audible>40&&!mixer.Playing(id));
 }
 for(auto bytes:{std::vector<char>(100,0),std::vector<char>{'I','D','3',0,0,0,0,0,0,0,0,0},std::vector<char>{char(0xff),char(0xfb),0,0,0,0,0,0,0,0,0,0},std::vector<char>{'R','I','F','F',0,0,0,0,'W','A','V','E'}}){bool rejected=false;try{NativeEngine::DecodeGameAudio(bytes.data(),bytes.size());}catch(const std::exception &){rejected=true;}check(rejected);}
 bool bounded=false;try{NativeEngine::DecodeGameAudio("ID3",64*1024*1024+1);}catch(const std::exception &){bounded=true;}check(bounded);
 std::cout<<"MPEG-1/2 Layer III speech rate, duration, finite/audible PCM, mono/stereo, mixer completion and malformed/bounded input PASS\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
