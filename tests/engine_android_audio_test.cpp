#include "audio_android.h"
#include <aaudio/AAudio.h>
#include <iostream>
#include <stdexcept>
#define check(v) do{if(!(v))throw std::runtime_error("AAudio fixture check "+std::to_string(__LINE__));}while(0)
struct AAudioStreamBuilder {DataCallback data=nullptr;ErrorCallback error=nullptr;void *user=nullptr;};
struct AAudioStreamStruct {AAudioStreamBuilder callbacks;};
namespace {int rate=44100,channels=2,format=AAUDIO_FORMAT_PCM_FLOAT,opened=0,closed=0,startResult=0;bool inError=false;AAudioStream *current=nullptr;}
aaudio_result_t AAudio_createStreamBuilder(AAudioStreamBuilder **b){*b=new AAudioStreamBuilder;return 0;}
void AAudioStreamBuilder_setDirection(AAudioStreamBuilder*,int32_t v){check(v==AAUDIO_DIRECTION_OUTPUT);}
void AAudioStreamBuilder_setFormat(AAudioStreamBuilder*,int32_t v){check(v==AAUDIO_FORMAT_PCM_FLOAT);}
void AAudioStreamBuilder_setChannelCount(AAudioStreamBuilder*,int32_t v){check(v==2);}
void AAudioStreamBuilder_setSampleRate(AAudioStreamBuilder*,int32_t v){check(v==48000);}
void AAudioStreamBuilder_setPerformanceMode(AAudioStreamBuilder*,int32_t){}
void AAudioStreamBuilder_setSharingMode(AAudioStreamBuilder*,int32_t){}
void AAudioStreamBuilder_setDataCallback(AAudioStreamBuilder*b,DataCallback cb,void*u){b->data=cb;b->user=u;}
void AAudioStreamBuilder_setErrorCallback(AAudioStreamBuilder*b,ErrorCallback cb,void*u){b->error=cb;b->user=u;}
aaudio_result_t AAudioStreamBuilder_openStream(AAudioStreamBuilder*b,AAudioStream**s){current=*s=new AAudioStreamStruct{*b};++opened;return 0;}
aaudio_result_t AAudioStreamBuilder_delete(AAudioStreamBuilder*b){delete b;return 0;}
int32_t AAudioStream_getFormat(AAudioStream*){return format;}int32_t AAudioStream_getChannelCount(AAudioStream*){return channels;}int32_t AAudioStream_getSampleRate(AAudioStream*){return rate;}int32_t AAudioStream_getState(AAudioStream*){return 4;}
aaudio_result_t AAudioStream_requestStart(AAudioStream*){return startResult;}aaudio_result_t AAudioStream_requestStop(AAudioStream*){return 0;}
aaudio_result_t AAudioStream_close(AAudioStream*s){check(!inError);++closed;delete s;current=nullptr;return 0;}
int main(){try{
 auto mixer=std::make_shared<NativeEngine::AudioMixer>();NativeEngine::AndroidAudioOutput output(mixer);check(output.Start()&&mixer->OutputRunning()&&opened==1);check(output.Start()&&opened==1);
 auto clip=std::make_shared<NativeEngine::AudioClip>();clip->rate=44100;clip->stereo={1,1,0,0,-1,-1,0,0};check(mixer->Play(clip,1,64,1));float pcm[8];check(current->callbacks.data(current,current->callbacks.user,pcm,4)==AAUDIO_CALLBACK_RESULT_CONTINUE);check(pcm[0]==1&&pcm[2]==0&&pcm[4]==-1); // Negotiated rate must reach the resampler.
 inError=true;current->callbacks.error(current,current->callbacks.user,-899);inError=false;check(!mixer->OutputRunning()&&closed==0);rate=48000;check(output.Start()&&mixer->OutputRunning()&&opened==2&&closed==1);check(output.Diagnostics().find("last-error=-899")!=std::string::npos);
 output.Stop();check(!mixer->OutputRunning()&&closed==2);channels=1;check(!output.Start()&&!mixer->OutputRunning()&&closed==3);channels=2;startResult=-777;check(!output.Start()&&closed==4);startResult=0;check(output.Start());output.Stop();
 std::cout<<"AAudio boundary fixture: negotiated-rate PCM, route-error marking, reopening outside callback, bad layout/start failure and repeated stop PASS; no device audio tested\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
