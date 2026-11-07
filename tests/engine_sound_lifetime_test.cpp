#include "sound.h"
#include "audio_native.h"
#include "audio_status.h"
#include "filesys.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <new>
#include <iostream>
#include <stdexcept>
#define check(v) do{if(!(v))throw std::runtime_error("Effect lifetime check at line "+std::to_string(__LINE__));}while(0)
int main(){try{
 auto root=std::filesystem::temp_directory_path()/("reign-audio-lifetime-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(root);
 struct Cleanup{std::filesystem::path p;~Cleanup(){std::filesystem::remove_all(p);}} cleanup{root};
 std::vector<unsigned char> bytes(44+1024);auto w16=[&](unsigned at,unsigned v){bytes[at]=v;bytes[at+1]=v>>8;};auto w32=[&](unsigned at,unsigned v){w16(at,v);w16(at+2,v>>16);};std::memcpy(bytes.data(),"RIFF",4);w32(4,unsigned(bytes.size()-8));std::memcpy(bytes.data()+8,"WAVEfmt ",8);w32(16,16);w16(20,1);w16(22,1);w32(24,8000);w32(28,16000);w16(32,2);w16(34,16);std::memcpy(bytes.data()+36,"data",4);w32(40,1024);for(unsigned at=44;at<bytes.size();at+=2)w16(at,8192);
 for(const char *name:{"attack.wav","impact.wav","loop.wav"}){std::ofstream f(root/name,std::ios::binary);f.write(reinterpret_cast<const char *>(bytes.data()),bytes.size());}
 FileSys::Init();check(FileSys::AddSrcDir("effects",root.string().c_str())&&FileSys::SetActiveStream("effects"));Sound::Init();Sound::Digital::Claim(8,0);auto mixer=NativeEngine::GetDigitalMixer();mixer->SetOutputRunning(true);
 using Effect=Sound::Digital::Effect;alignas(Effect) unsigned char storage[sizeof(Effect)];
 auto *effect=new(storage) Effect("attack.wav");check(effect->Valid()&&effect->Play2D(1)&&effect->Playing());Sound::Digital::EffectPtr tracked;tracked=effect;effect->~Effect();check(!tracked.Alive());
 float pcm[128];mixer->Render(pcm,64);check(pcm[0]>.2f); // A transient FX must not cancel the sound it just started.
 effect=new(storage) Effect("impact.wav");check(!effect->Playing());check(effect->Play2D(1));effect->~Effect();mixer->Render(pcm,64);check(pcm[0]>.4f); // Old association cannot alias the reused address.
 mixer->StopAll();effect=new(storage) Effect("loop.wav");check(effect->Play2D(1,0,1,0));effect->StopByEffect();effect->~Effect();mixer->Render(pcm,64);for(float sample:pcm)check(sample==0);
 effect=new(storage) Effect("missing.wav");check(!effect->Valid()&&!effect->Play2D());effect->~Effect();check(NativeEngine::DigitalDiagnostics().find("missing.wav")!=std::string::npos);
 Sound::Done();mixer->SetOutputRunning(false);FileSys::Done();std::cout<<"Original Effect/Reaper destruction preserves audible attack/impact PCM; address reuse, explicit loop stop and missing-resource diagnostics PASS\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
