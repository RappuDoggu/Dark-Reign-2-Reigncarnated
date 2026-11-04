#pragma once
#include <cstdint>
struct AAudioStreamStruct;using AAudioStream=AAudioStreamStruct;
struct AAudioStreamBuilder;
using aaudio_result_t=int32_t;using aaudio_data_callback_result_t=int32_t;
constexpr int AAUDIO_OK=0,AAUDIO_DIRECTION_OUTPUT=0,AAUDIO_FORMAT_PCM_FLOAT=2,AAUDIO_PERFORMANCE_MODE_LOW_LATENCY=12,AAUDIO_SHARING_MODE_SHARED=1,AAUDIO_CALLBACK_RESULT_STOP=1,AAUDIO_CALLBACK_RESULT_CONTINUE=0,AAUDIO_ERROR_INVALID_FORMAT=-882;
using DataCallback=aaudio_data_callback_result_t(*)(AAudioStream*,void*,void*,int32_t);
using ErrorCallback=void(*)(AAudioStream*,void*,aaudio_result_t);
aaudio_result_t AAudio_createStreamBuilder(AAudioStreamBuilder**);
void AAudioStreamBuilder_setDirection(AAudioStreamBuilder*,int32_t);
void AAudioStreamBuilder_setFormat(AAudioStreamBuilder*,int32_t);
void AAudioStreamBuilder_setChannelCount(AAudioStreamBuilder*,int32_t);
void AAudioStreamBuilder_setSampleRate(AAudioStreamBuilder*,int32_t);
void AAudioStreamBuilder_setPerformanceMode(AAudioStreamBuilder*,int32_t);
void AAudioStreamBuilder_setSharingMode(AAudioStreamBuilder*,int32_t);
void AAudioStreamBuilder_setDataCallback(AAudioStreamBuilder*,DataCallback,void*);
void AAudioStreamBuilder_setErrorCallback(AAudioStreamBuilder*,ErrorCallback,void*);
aaudio_result_t AAudioStreamBuilder_openStream(AAudioStreamBuilder*,AAudioStream**);
aaudio_result_t AAudioStreamBuilder_delete(AAudioStreamBuilder*);
int32_t AAudioStream_getFormat(AAudioStream*);int32_t AAudioStream_getChannelCount(AAudioStream*);int32_t AAudioStream_getSampleRate(AAudioStream*);int32_t AAudioStream_getState(AAudioStream*);
aaudio_result_t AAudioStream_requestStart(AAudioStream*);aaudio_result_t AAudioStream_requestStop(AAudioStream*);aaudio_result_t AAudioStream_close(AAudioStream*);
