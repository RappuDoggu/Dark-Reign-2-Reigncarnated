#include "reign/bink_movie.hpp"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>
namespace reign {
namespace {void checked(int result,const char *what){if(result<0){char msg[128];av_strerror(result,msg,sizeof(msg));throw std::runtime_error(std::string(what)+": "+msg);}}}
struct BinkMovie::Impl {
 std::vector<std::uint8_t> bytes,pixels;std::vector<std::int16_t> pcm;std::size_t position=0;
 AVFormatContext *format=nullptr;AVIOContext *io=nullptr;AVCodecContext *video=nullptr,*audio=nullptr;AVFrame *frame=nullptr;AVPacket *packet=nullptr;SwsContext *scale=nullptr;
 int vi=-1,ai=-1;unsigned rate=0,w=0,h=0;double fps=0,length=0;std::int64_t decoded=0;bool eof=false;
 static int read(void *p,std::uint8_t *dst,int count){auto &s=*static_cast<Impl*>(p);auto n=std::min<std::size_t>(count,s.bytes.size()-s.position);if(!n)return AVERROR_EOF;std::memcpy(dst,s.bytes.data()+s.position,n);s.position+=n;return int(n);}
 static std::int64_t seek(void *p,std::int64_t offset,int whence){auto &s=*static_cast<Impl*>(p);if(whence==AVSEEK_SIZE)return s.bytes.size();whence&=~AVSEEK_FORCE;std::int64_t base=whence==SEEK_SET?0:whence==SEEK_CUR?s.position:whence==SEEK_END?s.bytes.size():-1;if(base<0||offset < -base||offset>std::int64_t(s.bytes.size())-base)return AVERROR(EINVAL);s.position=base+offset;return s.position;}
 void dispose(){sws_freeContext(scale);av_packet_free(&packet);av_frame_free(&frame);avcodec_free_context(&video);avcodec_free_context(&audio);avformat_close_input(&format);if(io){av_freep(&io->buffer);avio_context_free(&io);}}
 explicit Impl(std::vector<std::uint8_t> data):bytes(std::move(data)){
  try{
   if(bytes.size()<44||bytes.size()>128*1024*1024||std::memcmp(bytes.data(),"BIK",3))throw std::runtime_error("Invalid/oversized Bink movie");
   auto u32=[&](std::size_t at){return std::uint32_t(bytes[at])|(std::uint32_t(bytes[at+1])<<8)|(std::uint32_t(bytes[at+2])<<16)|(std::uint32_t(bytes[at+3])<<24);};
   if(u32(20)==0||u32(24)==0||u32(20)>2048||u32(24)>2048||u32(8)==0||u32(8)>432000||std::uint64_t(u32(4))+8>bytes.size())throw std::runtime_error("Invalid Bink header bounds");
   auto buffer=static_cast<std::uint8_t*>(av_malloc(32768));if(!buffer)throw std::bad_alloc();io=avio_alloc_context(buffer,32768,0,this,read,nullptr,seek);if(!io){av_free(buffer);throw std::bad_alloc();}
   format=avformat_alloc_context();if(!format)throw std::bad_alloc();format->pb=io;format->flags|=AVFMT_FLAG_CUSTOM_IO;
   checked(avformat_open_input(&format,nullptr,av_find_input_format("bink"),nullptr),"Open Bink");checked(avformat_find_stream_info(format,nullptr),"Read Bink streams");
   auto open=[&](int i){auto par=format->streams[i]->codecpar;auto codec=avcodec_find_decoder(par->codec_id);if(!codec)throw std::runtime_error("Bink decoder unavailable");auto ctx=avcodec_alloc_context3(codec);if(!ctx)throw std::bad_alloc();try{checked(avcodec_parameters_to_context(ctx,par),"Bink parameters");ctx->thread_count=1;ctx->max_pixels=2048*2048;checked(avcodec_open2(ctx,codec,nullptr),"Open Bink codec");return ctx;}catch(...){avcodec_free_context(&ctx);throw;}};
   for(unsigned i=0;i<format->nb_streams;++i){auto p=format->streams[i]->codecpar;if(p->codec_type==AVMEDIA_TYPE_VIDEO&&vi<0){if(p->width<=0||p->height<=0||p->width>2048||p->height>2048)throw std::runtime_error("Unsupported movie dimensions");vi=i;video=open(i);}else if(p->codec_type==AVMEDIA_TYPE_AUDIO&&ai<0){ai=i;audio=open(i);}}
   if(!video)throw std::runtime_error("Bink movie has no video");w=video->width;h=video->height;fps=av_q2d(format->streams[vi]->avg_frame_rate);if(!std::isfinite(fps)||fps<=0||fps>120)throw std::runtime_error("Invalid Bink frame rate");
   length=double(format->streams[vi]->duration)*av_q2d(format->streams[vi]->time_base);if(!std::isfinite(length)||length<=0||length>3600)throw std::runtime_error("Invalid Bink duration");
   if(audio){rate=audio->sample_rate;if(rate<8000||rate>96000||audio->ch_layout.nb_channels<1||audio->ch_layout.nb_channels>2)throw std::runtime_error("Unsupported Bink audio");}
   frame=av_frame_alloc();packet=av_packet_alloc();if(!frame||!packet)throw std::bad_alloc();pixels.resize(std::size_t(w)*h*4);
  }catch(...){dispose();throw;}
 }
 ~Impl(){dispose();}
 bool next(){
  while(!eof){int result=av_read_frame(format,packet);if(result==AVERROR_EOF){eof=true;return false;}checked(result,"Read Bink packet");
   auto codec=packet->stream_index==vi?video:packet->stream_index==ai?audio:nullptr;if(!codec){av_packet_unref(packet);continue;}
   result=avcodec_send_packet(codec,packet);av_packet_unref(packet);checked(result,"Decode Bink packet");bool gotVideo=false;
   while((result=avcodec_receive_frame(codec,frame))>=0){
    if(codec==video){
     if(frame->width!=int(w)||frame->height!=int(h))throw std::runtime_error("Bink dimensions changed");
     scale=sws_getCachedContext(scale,w,h,static_cast<AVPixelFormat>(frame->format),w,h,AV_PIX_FMT_RGBA,SWS_BILINEAR,nullptr,nullptr,nullptr);if(!scale)throw std::runtime_error("Bink color conversion unavailable");
     std::uint8_t *dst[]={pixels.data(),nullptr,nullptr,nullptr};int stride[]={int(w*4),0,0,0};if(sws_scale(scale,frame->data,frame->linesize,0,h,dst,stride)!=int(h))throw std::runtime_error("Incomplete Bink image");++decoded;gotVideo=true;
    }else{
     int channels=frame->ch_layout.nb_channels;if(channels<1||channels>2||(frame->format!=AV_SAMPLE_FMT_FLT&&frame->format!=AV_SAMPLE_FMT_FLTP)||frame->sample_rate!=int(rate))throw std::runtime_error("Bink audio format changed");
     if(pcm.size()+std::size_t(frame->nb_samples)*2>rate*4u)throw std::runtime_error("Movie audio consumer fell behind");
     for(int i=0;i<frame->nb_samples;++i)for(int c=0;c<2;++c){int channel=std::min(c,channels-1);auto samples=reinterpret_cast<const float*>(frame->extended_data[frame->format==AV_SAMPLE_FMT_FLTP?channel:0]);float f=samples[frame->format==AV_SAMPLE_FMT_FLTP?i:i*channels+channel];if(!std::isfinite(f))throw std::runtime_error("Invalid Bink audio sample");pcm.push_back(static_cast<std::int16_t>(std::clamp(f*32768.f,-32768.f,32767.f)));}
    }
    av_frame_unref(frame);
   }
   if(result!=AVERROR(EAGAIN)&&result!=AVERROR_EOF)checked(result,"Receive Bink frame");if(gotVideo)return true;
  }return false;
 }
};
BinkMovie::BinkMovie(std::vector<std::uint8_t> b):impl_(std::make_unique<Impl>(std::move(b))){}
BinkMovie::~BinkMovie()=default;
unsigned BinkMovie::width()const{return impl_->w;}unsigned BinkMovie::height()const{return impl_->h;}unsigned BinkMovie::sample_rate()const{return impl_->rate;}double BinkMovie::duration()const{return impl_->length;}
std::int64_t BinkMovie::frame_index()const{return impl_->decoded-1;}
bool BinkMovie::advance(double seconds){auto &s=*impl_;if(!std::isfinite(seconds)||seconds<0)throw std::invalid_argument("Invalid movie time");if(seconds>=s.length)return false;auto target=static_cast<std::int64_t>(std::floor(seconds*s.fps));while(s.decoded<=target&&!s.eof)s.next();return s.decoded>0;}
void BinkMovie::restore_position(double seconds){
 if(frame_index()!=-1||!std::isfinite(seconds)||seconds<0||seconds>=duration())throw std::invalid_argument("Invalid movie restore position");
 for(double t=0;t<seconds;t+=1./60){advance(t);drain_audio();}
 advance(seconds);drain_audio();
}
const std::vector<std::uint8_t>& BinkMovie::rgba()const{return impl_->pixels;}
std::vector<std::int16_t> BinkMovie::drain_audio(){std::vector<std::int16_t> p;p.swap(impl_->pcm);return p;}
}
