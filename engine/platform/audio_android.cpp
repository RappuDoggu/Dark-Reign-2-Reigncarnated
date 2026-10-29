#include "audio_android.h"
#include <aaudio/AAudio.h>
#include <stdexcept>
namespace NativeEngine {
AndroidAudioOutput::AndroidAudioOutput(std::shared_ptr<AudioMixer> m):mixer(std::move(m)){if(!mixer)throw std::invalid_argument("Missing Android audio mixer");}
AndroidAudioOutput::~AndroidAudioOutput(){Stop();}
bool AndroidAudioOutput::Start(){
 if(stream)return mixer->OutputRunning();
 AAudioStreamBuilder *builder=nullptr;
 if(AAudio_createStreamBuilder(&builder)!=AAUDIO_OK)return false;
 AAudioStreamBuilder_setDirection(builder,AAUDIO_DIRECTION_OUTPUT);
 AAudioStreamBuilder_setFormat(builder,AAUDIO_FORMAT_PCM_FLOAT);
 AAudioStreamBuilder_setChannelCount(builder,2);
 AAudioStreamBuilder_setSampleRate(builder,48000);
 AAudioStreamBuilder_setPerformanceMode(builder,AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
 AAudioStreamBuilder_setSharingMode(builder,AAUDIO_SHARING_MODE_SHARED);
 AAudioStreamBuilder_setDataCallback(builder,+[](AAudioStream *,void *user,void *data,int32_t frames)->aaudio_data_callback_result_t{
  if(frames<0||frames>65536)return AAUDIO_CALLBACK_RESULT_STOP;
  static_cast<AndroidAudioOutput *>(user)->mixer->Render(static_cast<float *>(data),std::size_t(frames));
  return AAUDIO_CALLBACK_RESULT_CONTINUE;
 },this);
 AAudioStreamBuilder_setErrorCallback(builder,+[](AAudioStream *,void *user,aaudio_result_t){static_cast<AndroidAudioOutput *>(user)->mixer->SetOutputRunning(false);},this);
 const auto result=AAudioStreamBuilder_openStream(builder,&stream);
 AAudioStreamBuilder_delete(builder);
 if(result!=AAUDIO_OK){stream=nullptr;return false;}
 if(AAudioStream_getFormat(stream)!=AAUDIO_FORMAT_PCM_FLOAT||AAudioStream_getChannelCount(stream)!=2||AAudioStream_getSampleRate(stream)!=48000){Stop();return false;}
 mixer->SetOutputRunning(true);
 if(AAudioStream_requestStart(stream)!=AAUDIO_OK||!mixer->OutputRunning()){Stop();return false;}
 return true;
}
void AndroidAudioOutput::Stop()noexcept{
 mixer->SetOutputRunning(false);
 if(stream){AAudioStream_requestStop(stream);AAudioStream_close(stream);stream=nullptr;}
 mixer->StopAll();
}
}
