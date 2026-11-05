#include "sound.h"
#include "audio_native.h"
#include "audio_status.h"
#include "filesys.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#define check(v) do{if(!(v))throw std::runtime_error("Reserved speech check line "+std::to_string(__LINE__));}while(0)
int main(int argc,char **argv){try{
 check(argc==2);FileSys::Init();check(FileSys::AddSrcDir("speech",argv[1])&&FileSys::SetActiveStream("speech"));Sound::Init();auto mixer=NativeEngine::GetDigitalMixer();Sound::Digital::Claim(8,0);mixer->SetOutputRunning(true);
 using namespace Sound::Digital;
 auto effect=std::make_shared<NativeEngine::AudioClip>();effect->rate=48000;effect->stereo.assign(48000*2,0.01f);
 // Saturate ordinary FX: the separately reserved speech must still start.
 for(unsigned n=0;n<64;++n)check(mixer->Play(effect,1,64,1,1));
 Reserved::Submit("mission","speech-mono.mp3",Reserved::MODE2);Sound::Poll();check(NativeEngine::ReservedAudioPlaying());
 Reserved::Submit("mission","speech-stereo.mp3",Reserved::MODE2);
 // More combat FX cannot evict a reserved dialogue sample.
 for(unsigned n=0;n<100;++n)mixer->Play(effect,1,64,1,2);
 check(NativeEngine::ReservedAudioPlaying());float out[960];
 for(unsigned n=0;n<51;++n)mixer->Render(out,480);
 Sound::Poll();check(NativeEngine::ReservedAudioPlaying());
 auto report=NativeEngine::DigitalDiagnostics();check(report.find("speech-mono.mp3 1 1 0 0")!=std::string::npos&&report.find("speech-stereo.mp3 1 1 0 0")!=std::string::npos);
 for(unsigned n=0;n<65;++n)mixer->Render(out,480);Sound::Poll();check(!NativeEngine::ReservedAudioPlaying());
 Reserved::Submit("mission","speech-mono.mp3",Reserved::MODE2);Sound::Poll();check(NativeEngine::ReservedAudioPlaying());Reserved::Reset("mission");check(!NativeEngine::ReservedAudioPlaying());
 Reserved::Submit("mission","absent-dialogue.mp3",Reserved::MODE2);Sound::Poll();check(!NativeEngine::ReservedAudioPlaying());check(NativeEngine::DigitalDiagnostics().find("absent-dialogue.mp3 1 0 1")!=std::string::npos);
 Sound::Done();mixer->SetOutputRunning(false);FileSys::Done();std::cout<<"Original Reserved MP3 queue, combat saturation/priority protection, completion, reset and named missing diagnostics PASS\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
