#include "sound.h"
#include "audio_native.h"
#include "filesys.h"
#include <fstream>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#define check(v) do{if(!(v))throw std::runtime_error("Sound check failed at line "+std::to_string(__LINE__));}while(0)
int main(int argc,char **argv){if(argc!=3)return 2;try{
 FileSys::Init();check(FileSys::AddSrcPack("base",argv[1])&&FileSys::SetActiveStream("base"));
 std::ifstream names(argv[2]);check(bool(names));std::string name;unsigned count=0;std::size_t total=0;
 while(std::getline(names,name)){
  auto file=std::unique_ptr<FileSys::DataFile,void(*)(FileSys::DataFile*)>(FileSys::Open(name.c_str()),FileSys::Close);check(bool(file));
  auto clip=NativeEngine::DecodeWave(file->GetMemoryPtr(),file->Size());check(clip->rate&&clip->stereo.size());++count;total+=clip->stereo.size()/2;
 }
 check(count==519);Sound::Init();auto mixer=NativeEngine::GetDigitalMixer();Sound::Digital::Claim(8,0);
 check(!Sound::Digital::Claimed2D());mixer->SetOutputRunning(true);check(Sound::Digital::Claimed2D());
 float buffer[1024];bool audible=false;
 {
  auto effect=std::make_unique<Sound::Digital::Effect>("comp.powerdown.wav");check(effect->Valid());
  Sound::Digital::EffectPtr tracked;tracked=effect.get();check(tracked.Alive());
  check(effect->Play2D(1,12,1,1,0)&&effect->Playing());
  for(int block=0;block<300&&effect->Playing();++block){mixer->Render(buffer,512);for(int i=0;i<1024;i+=2){audible|=std::abs(buffer[i])>0.0001f;check(buffer[i+1]==0);}}
  check(audible&&!effect->Playing());
  // A new instance verifies destruction invalidates original Reaper handles.
  effect.reset();check(!tracked.Alive());
 }
 Sound::Digital::Reserved::Submit("test.voice","comp.powerdown.wav",Sound::Digital::Reserved::MODE4);
 Sound::Poll();mixer->Render(buffer,512);
 bool reservedPcm=false;for(float sample:buffer)reservedPcm|=std::abs(sample)>0.0001f;check(reservedPcm);
 Sound::Digital::Reserved::Reset(nullptr);mixer->Render(buffer,512);for(float sample:buffer)check(sample==0);
 check(Sound::Digital::Stream::Start("ambient","snow_wind.wav")&&Sound::Digital::Stream::IsPlaying("ambient"));
 Sound::Digital::Stream::Stop("ambient");check(!Sound::Digital::Stream::IsPlaying("ambient"));
 Sound::Digital::Stop();Sound::Digital::ClearRecords();Sound::Done();mixer->SetOutputRunning(false);FileSys::Done();
 std::cout<<"Decoded all "<<count<<" owned PCM effects ("<<total<<" source frames); original Effect playback/pan/completion, Reaper destruction and reserved reset pass. Host PCM only.\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
