#include "reign/tga.hpp"
#include <stdexcept>
namespace reign {
RgbaImage decode_tga(const std::vector<std::uint8_t> &b){
 if(b.size()<18)throw std::runtime_error("Truncated TGA header");
 if(b[1] || (b[2]!=2&&b[2]!=10) || (b[16]!=24&&b[16]!=32))throw std::runtime_error("Unsupported TGA color map/type/depth");
 const std::uint32_t w=b[12]|(b[13]<<8),h=b[14]|(b[15]<<8),step=b[16]/8;
 if(!w||!h||w>4096||h>4096||(b[17]&0xc0))throw std::runtime_error("Invalid/interleaved TGA dimensions");
 RgbaImage image{w,h,std::vector<std::uint8_t>(std::size_t(w)*h*4)};
 std::size_t at=18+b[0],pixel=0;
 auto put=[&](const std::uint8_t *p){
  auto x=pixel%w,y=pixel/w;if(b[17]&0x10)x=w-1-x;if(!(b[17]&0x20))y=h-1-y;
  auto out=(y*w+x)*4;image.pixels[out]=p[2];image.pixels[out+1]=p[1];image.pixels[out+2]=p[0];image.pixels[out+3]=step==4?p[3]:255;++pixel;
 };
 auto need=[&](std::size_t size){if(at>b.size()||size>b.size()-at)throw std::runtime_error("Truncated TGA payload");};
 while(pixel<std::size_t(w)*h){
  std::size_t count=1;bool repeated=false;
  if(b[2]==10){need(1);auto tag=b[at++];count=(tag&127)+1;repeated=tag&128;}
  if(count>std::size_t(w)*h-pixel)throw std::runtime_error("TGA packet exceeds image");
  need(repeated?step:count*step);
  for(std::size_t n=0;n<count;++n){put(b.data()+at);if(!repeated)at+=step;}
  if(repeated)at+=step;
 }
 return image;
}
}
