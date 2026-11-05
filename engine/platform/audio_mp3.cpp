#include "audio_native.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace NativeEngine {
namespace {
void checked(int result,const char *what){if(result<0){char message[128];av_strerror(result,message,sizeof(message));throw std::runtime_error(std::string(what)+": "+message);}}
// No URL, filename, external stream or network protocol is exposed to FFmpeg.
struct SpeechDecoder {
 const std::uint8_t *bytes;std::size_t size,position=0;
 AVFormatContext *format=nullptr;AVIOContext *io=nullptr;AVCodecContext *codec=nullptr;AVFrame *frame=nullptr;AVPacket *packet=nullptr;
 SpeechDecoder(const void *p,std::size_t n):bytes(static_cast<const std::uint8_t *>(p)),size(n){}
 ~SpeechDecoder(){av_packet_free(&packet);av_frame_free(&frame);avcodec_free_context(&codec);avformat_close_input(&format);if(io){av_freep(&io->buffer);avio_context_free(&io);}}
 static int read(void *opaque,std::uint8_t *dst,int count){auto &s=*static_cast<SpeechDecoder *>(opaque);if(count<=0)return AVERROR(EINVAL);auto n=std::min<std::size_t>(count,s.size-s.position);if(!n)return AVERROR_EOF;std::memcpy(dst,s.bytes+s.position,n);s.position+=n;return int(n);}
 static std::int64_t seek(void *opaque,std::int64_t offset,int whence){auto &s=*static_cast<SpeechDecoder *>(opaque);if(whence==AVSEEK_SIZE)return s.size;whence&=~AVSEEK_FORCE;std::int64_t base=whence==SEEK_SET?0:whence==SEEK_CUR?s.position:whence==SEEK_END?s.size:-1;if(base<0||offset < -base||offset>std::int64_t(s.size)-base)return AVERROR(EINVAL);s.position=base+offset;return s.position;}
};
}
std::shared_ptr<const AudioClip> DecodeMp3(const void *input,std::size_t size){
 if(!input||size<12||size>64*1024*1024)throw std::invalid_argument("Invalid MP3 size");
 SpeechDecoder s(input,size);
 auto buffer=static_cast<std::uint8_t *>(av_malloc(32768));if(!buffer)throw std::bad_alloc();
 s.io=avio_alloc_context(buffer,32768,0,&s,SpeechDecoder::read,nullptr,SpeechDecoder::seek);if(!s.io){av_free(buffer);throw std::bad_alloc();}
 s.format=avformat_alloc_context();if(!s.format)throw std::bad_alloc();s.format->pb=s.io;s.format->flags|=AVFMT_FLAG_CUSTOM_IO;s.format->probesize=1024*1024;s.format->max_analyze_duration=AV_TIME_BASE;
 const auto demuxer=av_find_input_format("mp3");if(!demuxer)throw std::runtime_error("MP3 demuxer unavailable; rebuild the pinned FFmpeg recipe");
 checked(avformat_open_input(&s.format,nullptr,demuxer,nullptr),"Open MP3 speech");
 checked(avformat_find_stream_info(s.format,nullptr),"Read MP3 speech");
 if(s.format->nb_streams!=1||s.format->streams[0]->codecpar->codec_id!=AV_CODEC_ID_MP3)throw std::runtime_error("Expected one MPEG Layer III speech stream");
 auto decoder=avcodec_find_decoder(AV_CODEC_ID_MP3);if(!decoder)throw std::runtime_error("MP3 speech decoder unavailable; rebuild FFmpeg with mp3float");
 s.codec=avcodec_alloc_context3(decoder);if(!s.codec)throw std::bad_alloc();
 checked(avcodec_parameters_to_context(s.codec,s.format->streams[0]->codecpar),"MP3 parameters");s.codec->thread_count=1;
 checked(avcodec_open2(s.codec,decoder,nullptr),"Open MP3 decoder");
 auto clip=std::make_shared<AudioClip>();clip->rate=s.codec->sample_rate;
 if(clip->rate<8000||clip->rate>96000||s.codec->ch_layout.nb_channels<1||s.codec->ch_layout.nb_channels>2)throw std::runtime_error("Unsupported MP3 rate/channels");
 s.frame=av_frame_alloc();s.packet=av_packet_alloc();if(!s.frame||!s.packet)throw std::bad_alloc();
 auto receive=[&](){int result;while((result=avcodec_receive_frame(s.codec,s.frame))>=0){
  auto f=s.frame;int channels=f->ch_layout.nb_channels;bool planar=f->format==AV_SAMPLE_FMT_FLTP;
  if((!planar&&f->format!=AV_SAMPLE_FMT_FLT)||f->sample_rate!=int(clip->rate)||channels<1||channels>2||f->nb_samples<0)throw std::runtime_error("MP3 speech format changed");
  if(clip->stereo.size()+std::size_t(f->nb_samples)*2>64*1024*1024/sizeof(float))throw std::runtime_error("Decoded speech exceeds 64 MiB");
  for(int i=0;i<f->nb_samples;++i)for(int c=0;c<2;++c){int channel=std::min(c,channels-1);auto samples=reinterpret_cast<const float *>(f->extended_data[planar?channel:0]);float value=samples[planar?i:i*channels+channel];if(!std::isfinite(value))throw std::runtime_error("Invalid MP3 speech sample");clip->stereo.push_back(std::clamp(value,-1.f,1.f));}
  av_frame_unref(f);
 }if(result!=AVERROR(EAGAIN)&&result!=AVERROR_EOF)checked(result,"Receive MP3 speech");};
 unsigned packets=0;int result;
 while((result=av_read_frame(s.format,s.packet))>=0){if(++packets>100000)throw std::runtime_error("MP3 packet limit exceeded");checked(avcodec_send_packet(s.codec,s.packet),"Decode MP3 speech");av_packet_unref(s.packet);receive();}
 if(result!=AVERROR_EOF)checked(result,"Read MP3 packet");checked(avcodec_send_packet(s.codec,nullptr),"Flush MP3 speech");receive();
 if(clip->stereo.empty())throw std::runtime_error("MP3 speech has no samples");return clip;
}
}
