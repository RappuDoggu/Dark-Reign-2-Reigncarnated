#include "reign/bitmap_loader.hpp"
#include "reign/bitmap_pixels.hpp"
#include <array>
#include <stdexcept>
namespace reign {
RgbaImage load_bitmap_pic(const std::vector<std::uint8_t> &b){
 // Softimage PIC layout/channel order inspected in original Bitmap::ReadPIC.
 if(b.size()<108||b[0]!=0x53||b[1]!=0x80||b[2]!=0xf6||b[3]!=0x34||
    b[88]!='P'||b[89]!='I'||b[90]!='C'||b[91]!='T')throw std::runtime_error("Invalid PIC header");
 const unsigned w=(unsigned(b[92])<<8)|b[93],h=(unsigned(b[94])<<8)|b[95];
 if(!w||!h||w>4096||h>4096)throw std::runtime_error("Invalid PIC dimensions");
 std::size_t at=104;
 auto need=[&](std::size_t n){if(at>b.size()||n>b.size()-at)throw std::runtime_error("Truncated PIC payload");};
 auto channel=[&](){need(4);std::array<std::uint8_t,4> c{b[at],b[at+1],b[at+2],b[at+3]};at+=4;
  if(c[0]>1||c[1]!=8||(c[2]!=0&&c[2]!=2))throw std::runtime_error("Unsupported PIC channel encoding");return c;};
 auto rgb=channel();if(rgb[3]!=0xe0)throw std::runtime_error("PIC RGB channel required");
 const bool alpha=rgb[0];std::array<std::uint8_t,4> a{};
 if(alpha){a=channel();if(a[0]||a[3]!=0x10)throw std::runtime_error("Unsupported PIC channel chain");}
 const unsigned step=alpha?4:3;std::vector<std::uint8_t> rows(std::size_t(w)*h*step);
 auto read_pixel=[&](std::uint8_t *p,unsigned bytes){need(bytes);for(unsigned k=0;k<bytes;++k)p[bytes-1-k]=b[at++];};
 for(unsigned y=0;y<h;++y){
  auto decode=[&](unsigned encoding,unsigned channels,unsigned offset){
   unsigned x=0;
   while(x<w){
    unsigned count=1;bool repeat=false;
    if(encoding==2){need(1);const auto tag=b[at++];if(tag<128)count=tag+1;
     else{repeat=true;if(tag==128){need(2);count=(unsigned(b[at])<<8)|b[at+1];at+=2;}else count=tag-127;}}
    if(!count||count>w-x)throw std::runtime_error("PIC packet exceeds scanline");
    std::array<std::uint8_t,3> pixel{};if(repeat)read_pixel(pixel.data(),channels);
    for(unsigned i=0;i<count;++i){auto *p=rows.data()+(std::size_t(y)*w+x++)*step+offset;
     if(repeat)for(unsigned k=0;k<channels;++k)p[k]=pixel[k];else read_pixel(p,channels);}
   }
  };
  decode(rgb[2],3,0);if(alpha)decode(a[2],1,3);
 }
 return {w,h,convert_bitmap_pixels(w,h,step*8,rows,rgba8_format)};
}
}
