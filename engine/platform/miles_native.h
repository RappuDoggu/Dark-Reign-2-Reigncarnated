#pragma once
#include "audio_native.h"
// Internal source-level sample handles, never the proprietary Miles ABI.
using HDIGDRIVER=NativeEngine::AudioMixer *;
using HPROVIDER=void *;
struct NativeSample;
using HSAMPLE=NativeSample *;
constexpr int SMP_DONE=2,SMP_PLAYING=4;
HSAMPLE AIL_allocate_sample_handle(HDIGDRIVER);
void AIL_release_sample_handle(HSAMPLE);
void AIL_init_sample(HSAMPLE);
int NativeSetSampleFile(HSAMPLE,const void *,unsigned);
int AIL_set_named_sample_file(HSAMPLE,const char *,const void *,unsigned,int);
void AIL_set_sample_volume(HSAMPLE,int);
void AIL_start_sample(HSAMPLE);
void AIL_end_sample(HSAMPLE);
int AIL_sample_status(HSAMPLE);
namespace NativeEngine {const char *DigitalLastError();}
