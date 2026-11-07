#include "audio_android.h"
#include <aaudio/AAudio.h>
#include <stdexcept>
#include <sstream>
namespace NativeEngine {
AndroidAudioOutput::AndroidAudioOutput(std::shared_ptr<AudioMixer> m):mixer(std::move(m)){if(!mixer)throw std::invalid_argument("Missing Android audio mixer");}
AndroidAudioOutput::~AndroidAudioOutput(){Stop();}
bool AndroidAudioOutput::Start(){
 if(stream&&mixer->OutputRunning())return true;
 if(stream)Stop(); // Disconnection callbacks only mark failure; close/reopen here.
 ++attempts;actualRate=0;
 AAudioStreamBuilder *builder=nullptr;
 auto creation=AAudio_createStreamBuilder(&builder);if(creation!=AAUDIO_OK){lastError=creation;return false;}
 AAudioStreamBuilder_setDirection(builder,AAUDIO_DIRECTION_OUTPUT);
 AAudioStreamBuilder_setFormat(builder,AAUDIO_FORMAT_PCM_FLOAT);
 AAudioStreamBuilder_setChannelCount(builder,2);
 AAudioStreamBuilder_setSampleRate(builder,48000);
 AAudioStreamBuilder_setPerformanceMode(builder,AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
 AAudioStreamBuilder_setSharingMode(builder,AAUDIO_SHARING_MODE_SHARED);
 AAudioStreamBuilder_setDataCallback(builder,+[](AAudioStream *stream,void *user,void *data,int32_t frames)->aaudio_data_callback_result_t{
  if(frames<0||frames>65536)return AAUDIO_CALLBACK_RESULT_STOP;
  static_cast<AndroidAudioOutput *>(user)->mixer->Render(static_cast<float *>(data),std::size_t(frames),unsigned(AAudioStream_getSampleRate(stream)));
  return AAUDIO_CALLBACK_RESULT_CONTINUE;
 },this);
 AAudioStreamBuilder_setErrorCallback(builder,+[](AAudioStream *,void *user,aaudio_result_t error){auto *output=static_cast<AndroidAudioOutput *>(user);output->lastError=error;++output->errors;output->mixer->SetOutputRunning(false);},this);
 const auto result=AAudioStreamBuilder_openStream(builder,&stream);
 AAudioStreamBuilder_delete(builder);
 if(result!=AAUDIO_OK){lastError=result;stream=nullptr;return false;}
 actualRate=unsigned(AAudioStream_getSampleRate(stream));
 if(AAudioStream_getFormat(stream)!=AAUDIO_FORMAT_PCM_FLOAT||AAudioStream_getChannelCount(stream)!=2||actualRate<8000||actualRate>192000){lastError=AAUDIO_ERROR_INVALID_FORMAT;Stop();return false;}
 mixer->SetOutputRunning(true);
 const auto started=AAudioStream_requestStart(stream);if(started!=AAUDIO_OK||!mixer->OutputRunning()){if(started!=AAUDIO_OK)lastError=started;Stop();return false;}
 return true;
}
void AndroidAudioOutput::Stop()noexcept{
 mixer->SetOutputRunning(false);
 if(stream){AAudioStream_requestStop(stream);AAudioStream_close(stream);stream=nullptr;}
 mixer->StopAll();
}
std::string AndroidAudioOutput::Diagnostics()const{std::ostringstream out;out<<"AAudio running="<<mixer->OutputRunning()<<" rate="<<actualRate<<" attempts="<<attempts<<" callback-errors="<<errors.load()<<" last-error="<<lastError.load()<<" state="<<(stream?int(AAudioStream_getState(stream)):-1);return out.str();}
}
