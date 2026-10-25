#include "reign/tga_source.hpp"
#include <algorithm>
#include <stdexcept>
namespace reign {
TgaSource decode_tga_source(const std::vector<std::uint8_t> &b){
 if(b.size()<18)throw std::runtime_error("Truncated TGA header");
 auto word=[&](unsigned n){return unsigned(b[n])|(unsigned(b[n+1])<<8);};
 const bool indexed=b[2]==1||b[2]==9,rle=b[2]==9||b[2]==10;
 if((!indexed&&b[2]!=2&&b[2]!=10)||b[1]!=(indexed?1:0)||
    (indexed?b[16]!=8:(b[16]!=16&&b[16]!=24&&b[16]!=32)))
  throw std::runtime_error("Unsupported TGA color map/type/depth");
 const auto w=word(12),h=word(14),step=unsigned(b[16]/8);
 if(!w||!h||w>4096||h>4096||(b[17]&0xc0))throw std::runtime_error("Invalid/interleaved TGA dimensions");
 std::size_t at=18;
 auto need=[&](std::size_t n){if(at>b.size()||n>b.size()-at)throw std::runtime_error("Truncated TGA payload");};
 need(b[0]);at+=b[0];
 TgaSource out{w,h,b[16],{}, {}};
 const auto first=word(3),count=word(5),palette_step=unsigned(b[7]/8);
 if(indexed){
  if(!count||first>=256||count>256-first||(b[7]!=16&&b[7]!=24&&b[7]!=32))throw std::runtime_error("Invalid TGA palette");
  need(std::size_t(count)*palette_step);out.palette.resize(256);
  for(unsigned i=0;i<count;++i){
   const auto *p=b.data()+at;auto &color=out.palette[first+i];
   if(palette_step==2){const unsigned v=p[0]|(unsigned(p[1])<<8);
    // Matches the inspected original ReadTGA palette extraction, including its channel order.
    color={std::uint8_t((v&31)<<3),std::uint8_t((v&992)>>2),std::uint8_t((v&31744)>>7),255};
   }else color={p[2],p[1],p[0],255}; // Original CopyLine ignores palette alpha.
   at+=palette_step;
  }
 }else if(first||count||b[7])throw std::runtime_error("Unexpected TGA palette metadata");
 const std::size_t total=std::size_t(w)*h;out.rows.resize(total*step);
 std::size_t pixel=0;
 auto put=[&](const std::uint8_t *p){
  if(indexed&&(p[0]<first||p[0]>=first+count))throw std::runtime_error("TGA palette index outside declared range");
  auto x=pixel%w,y=pixel/w;if(b[17]&0x10)x=w-1-x;if(!(b[17]&0x20))y=h-1-y;
  std::copy_n(p,step,out.rows.data()+(y*w+x)*step);++pixel;
 };
 while(pixel<total){
  unsigned n=1;bool repeat=false;
  if(rle){need(1);const auto tag=b[at++];n=(tag&127)+1;repeat=tag&128;}
  if(n>total-pixel)throw std::runtime_error("TGA packet exceeds image");
  need(repeat?step:std::size_t(n)*step);
  for(unsigned i=0;i<n;++i){put(b.data()+at);if(!repeat)at+=step;}
  if(repeat)at+=step;
 }
 return out;
}
}
