// Compiled in place of the platform-specific digital driver unit. Original
// Effect, Record and Reserved implementations still own their engine behavior.
#include "sound_private.h"
#include "audio_native.h"
#include "audio_status.h"
#include "music_native.h"
#include <chrono>
#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <array>
#include <sstream>
#include <set>
#include <limits>
namespace {
std::string audioError;
bool digitalInitialized=false,digitalDisabled=false;
unsigned voiceLimit=0;
std::map<Sound::Digital::Record *,std::shared_ptr<const NativeEngine::AudioClip>> clips;
std::map<std::string,std::unique_ptr<Sound::Digital::Effect>> streams;
struct ResourceAudio {std::string name;std::array<std::uint64_t,6> counts{};std::uint64_t used=0;};
std::array<ResourceAudio,64> audioResources;std::uint64_t audioTick=0,audioEvicted=0;
std::shared_ptr<const NativeEngine::AudioClip> loadClip(FileSys::FastFind *file){
 if(!file||!file->StreamAlive())throw std::runtime_error("Audio resource stream is no longer available");
 auto data=std::unique_ptr<FileSys::DataFile,void(*)(FileSys::DataFile*)>(FileSys::Open(file),FileSys::Close);
 if(!data)throw std::runtime_error("Cannot open audio resource");
 return NativeEngine::DecodeGameAudio(data->GetMemoryPtr(),data->Size());
}
}
namespace NativeEngine {
const char *DigitalLastError(){return audioError.c_str();}
void NoteAudio(const char *name,AudioEvent event){std::string resource=name?name:"[unknown]";if(resource.size()>192)resource.resize(192);auto slot=std::find_if(audioResources.begin(),audioResources.end(),[&](const auto &row){return row.name==resource;});if(slot==audioResources.end()){slot=std::min_element(audioResources.begin(),audioResources.end(),[](const auto &a,const auto &b){return a.used<b.used;});if(slot->used)++audioEvicted;*slot={resource,{},0};}++slot->counts[unsigned(event)];slot->used=++audioTick;}
std::string DigitalDiagnostics(){std::ostringstream out;out<<GetDigitalMixer()->Diagnostics()<<"\nDigital initialized="<<digitalInitialized<<" disabled="<<digitalDisabled<<" claimed="<<voiceLimit<<" reserved-playing="<<ReservedAudioPlaying()<<" cached="<<clips.size()<<" last-error="<<audioError<<"\nRecent resource requests/started/missing/decode-failed/suppressed/repeat (evicted="<<audioEvicted<<"):\n";for(const auto &row:audioResources)if(row.used){out<<row.name;for(auto count:row.counts)out<<' '<<count;out<<'\n';}return out.str();}
}
struct NativeSample {NativeEngine::AudioMixer *mixer;std::shared_ptr<const NativeEngine::AudioClip> clip;std::uint64_t id=0;float volume=1;std::string name;};
namespace {std::set<NativeSample *> reservedSamples;}
namespace NativeEngine {bool ReservedAudioPlaying(){for(auto s:reservedSamples)if(s->mixer->Playing(s->id))return true;return false;}}
HSAMPLE AIL_allocate_sample_handle(HDIGDRIVER mixer){if(!mixer)return nullptr;try{auto s=std::make_unique<NativeSample>();s->mixer=mixer;reservedSamples.insert(s.get());return s.release();}catch(...){return nullptr;}}
void AIL_end_sample(HSAMPLE s){if(s){s->mixer->Stop(s->id);s->id=0;}}
void AIL_release_sample_handle(HSAMPLE s){if(s){AIL_end_sample(s);reservedSamples.erase(s);delete s;}}
void AIL_init_sample(HSAMPLE s){if(s){AIL_end_sample(s);s->clip.reset();s->volume=1;}}
int NativeSetSampleFile(HSAMPLE s,const void *data,unsigned size,const char *name){if(!s)return 0;s->name=name?name:"[reserved sample]";try{s->clip=NativeEngine::DecodeGameAudio(data,size);return 1;}catch(const std::exception &e){audioError=s->name+": "+e.what();NativeEngine::NoteAudio(s->name.c_str(),NativeEngine::AudioEvent::decode_failed);s->clip.reset();return 0;}}
int AIL_set_named_sample_file(HSAMPLE s,const char *,const void *data,unsigned size,int){return NativeSetSampleFile(s,data,size);}
void AIL_set_sample_volume(HSAMPLE s,int volume){if(s)s->volume=std::clamp(volume,0,127)/127.f;}
// Original Reserved voices have dedicated handles, protected from ordinary FX stealing.
void AIL_start_sample(HSAMPLE s){if(s){s->id=s->mixer->Play(s->clip,s->volume,64,1,std::numeric_limits<float>::max());NativeEngine::NoteAudio(s->name.c_str(),s->id?NativeEngine::AudioEvent::started:NativeEngine::AudioEvent::suppressed);}}
int AIL_sample_status(HSAMPLE s){return s&&s->mixer->Playing(s->id)?SMP_PLAYING:SMP_DONE;}
namespace Sound { namespace Digital {
void Init(){if(digitalInitialized)return;digitalInitialized=true;digitalDisabled=false;voiceLimit=0;Reserved::Init();}
Bool Initialized(){return digitalInitialized;}
Bool Disabled(){return digitalDisabled;}
void Stop(){NativeEngine::GetDigitalMixer()->StopAll();Stream::Stop(nullptr);Reserved::Reset(nullptr);}
void ClearRecords(){Stop();clips.clear();Record::ReleaseAll();}
void Release(){Stop();Reserved::Release();voiceLimit=0;}
void Done(){if(!digitalInitialized)return;Release();Reserved::Done();ClearRecords();digitalInitialized=false;}
void CriticalShutdown(){Release();NativeEngine::GetDigitalMixer()->SetOutputRunning(false);}
void Claim(U32 max2D,U32 max3D){if(max3D)audioError="Positional 3D audio is not available yet";voiceLimit=std::min<U32>(max2D,64);}
Bool Claimed2D(){return digitalInitialized&&!digitalDisabled&&voiceLimit&&NativeEngine::GetDigitalMixer()->OutputRunning();}
Bool Claimed3D(){return FALSE;}
HDIGDRIVER GetDriver(){return Claimed2D()?NativeEngine::GetDigitalMixer().get():nullptr;}
S32 ConvertVolume(F32 value){return std::isfinite(value)?S32(std::clamp(value,0.f,1.f)*127.f+0.5f):0;}
void SetDisabled(Bool value){digitalDisabled=value;if(value)Stop();}
void SetVolume(F32 value){NativeEngine::GetDigitalMixer()->SetVolume(value);}
F32 Volume(){return NativeEngine::GetDigitalMixer()->Volume();}
const char *GetServiceName2D(){return Claimed2D()?"Native PCM mixer":"No current service";}
namespace Output {
Bool Play2D(Effect *effect,Record *record,S32 volume,U32 owner,F32 priority,S32 loops,S32 pan){
 if(!Claimed2D()||!record||!record->valid)return FALSE;
 try{
  auto found=clips.find(record);
  if(found==clips.end())found=clips.emplace(record,loadClip(record->fastFind)).first;
  const auto now=U32(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
  if(record->lastUse&&U32(now-U32(record->lastUse))<MIN_REPEATTIME){NativeEngine::NoteAudio(record->Name(),NativeEngine::AudioEvent::repeat);return FALSE;}
  if(!NativeEngine::GetDigitalMixer()->Play(found->second,volume/127.f,pan,loops,priority,owner,effect,record)){NativeEngine::NoteAudio(record->Name(),NativeEngine::AudioEvent::suppressed);return FALSE;}
  NativeEngine::NoteAudio(record->Name(),NativeEngine::AudioEvent::started);
  record->lastUse=S32(now);return TRUE;
 }catch(const std::exception &e){audioError=e.what();NativeEngine::NoteAudio(record->Name(),NativeEngine::AudioEvent::decode_failed);return FALSE;}
}
Bool UpdateEffect2D(Effect *effect,S32 pan,F32 priority,S32 volume){return NativeEngine::GetDigitalMixer()->UpdateEffect(effect,volume/127.f,pan,priority);}
Bool EffectPlaying(Effect *effect){return NativeEngine::GetDigitalMixer()->EffectPlaying(effect);}
Bool RecordInUse(Record *record){return NativeEngine::GetDigitalMixer()->RecordPlaying(record);}
void StopByEffect(Effect *effect){NativeEngine::GetDigitalMixer()->StopEffect(effect);}
void StopByRecord(Record *record){NativeEngine::GetDigitalMixer()->StopRecord(record);}
void StopByOwner(U32 owner){NativeEngine::GetDigitalMixer()->StopOwner(owner);}
void StopAllVoices(){NativeEngine::GetDigitalMixer()->StopAll();}
void Stop(){StopAllVoices();}
Bool Play3D(Effect *,Record *,S32,U32,F32,S32,F32,F32,F32,F32,F32){audioError="Positional 3D audio is not available yet";return FALSE;}
Bool UpdateEffect3D(Effect *,F32,F32,F32,F32,F32,F32,F32,S32){return FALSE;}
}
namespace Stream {
Bool Start(const char *ident,const char *file){
 if(!ident||!file||!Claimed2D())return FALSE;
 auto effect=std::make_unique<Effect>(file);
 if(!effect->Play2D(1))return FALSE;
 auto &slot=streams[ident];if(slot)slot->StopByEffect();slot=std::move(effect);return TRUE;
}
void Stop(const char *ident){if(ident){auto it=streams.find(ident);if(it!=streams.end()){it->second->StopByEffect();streams.erase(it);}}else{for(auto &item:streams)item.second->StopByEffect();streams.clear();}}
Bool IsPlaying(const char *ident){if(!ident)return FALSE;auto it=streams.find(ident);return it!=streams.end()&&it->second->Playing();}
}
} }
namespace Sound {namespace Redbook {
void Init(){Stop();SetMinTrack(2);}
void Done(){Stop();}
void CriticalShutdown(){Stop();}
} }
namespace Sound {namespace Digital {
Bool Next3DProvider(const char *&name,Bool &first){name=nullptr;first=FALSE;return FALSE;}
Bool SetProviderPreference(U32){return FALSE;}
Bool Change3DProvider(Bool){return FALSE;}
U32 GetServiceIndex3D(){return U32(-1);}
const char *GetServiceName3D(){return "3D positional audio unavailable";}
}}
namespace Sound {namespace Digital {
U32 CPUPercent(){throw std::runtime_error("Native mixer CPU telemetry is not available");}
}}
