#include "reign/bitmap_pixels.hpp"
#include <iostream>
#include <stdexcept>
namespace {
using U8=std::uint8_t;using U32=std::uint32_t;
void check(bool b,const char *s){if(!b)throw std::runtime_error(s);}
template<class F>void rejects(F f){try{f();}catch(const std::runtime_error&){return;}throw std::runtime_error("Invalid bitmap conversion accepted");}
U32 pack_channel(U8 value,U32 mask){
 if(!mask)return 0;unsigned shift=0,bits=0;while(!(mask&1)){++shift;mask>>=1;}while(mask){++bits;mask>>=1;}
 return (U32(value)/(U32(1)<<(8-bits)))*(U32(1)<<shift);
}
}
int main(){try{
 const reign::PixelFormat formats[]={reign::rgba8_format,reign::rgb565_format,{16,0x7c00,0x3e0,0x1f,0x8000},{16,0xf00,0xf0,0xf,0xf000},{24,0xff0000,0xff00,0xff,0},{32,0xff0000,0xff00,0xff,0xff000000}};
 std::vector<std::array<U8,4>> palette(256);for(unsigned i=0;i<256;++i)palette[i]={U8(i),U8(255-i),U8(i*17),0};
 unsigned comparisons=0;
 for(auto depth:{8u,16u,24u,32u}){
  std::vector<U8> source;std::vector<std::array<U8,4>> expected;
  for(unsigned i=0;i<257*3;++i){U8 r=U8(i*37),g=U8(i*71),b=U8(i*13),a=U8(i);
   if(depth==8){source.push_back(U8(i));expected.push_back({palette[U8(i)][0],palette[U8(i)][1],palette[U8(i)][2],255});}
   else if(depth==16){U32 raw=(U32(i)*131)&65535;source.push_back(U8(raw));source.push_back(U8(raw>>8));expected.push_back({U8((raw&0x7c00)>>7),U8((raw&0x3e0)>>2),U8((raw&31)<<3),U8((raw&0x8000)>>8)});}
   else {source.insert(source.end(),{b,g,r});if(depth==32)source.push_back(a);expected.push_back({r,g,b,U8(depth==32?a:255)});}
  }
  for(auto format:formats){auto output=reign::convert_bitmap_pixels(257,3,depth,source,format,palette);const auto bytes=format.depth/8;
   for(unsigned i=0;i<expected.size();++i){const auto c=expected[i];U32 golden=pack_channel(c[0],format.red)|pack_channel(c[1],format.green)|pack_channel(c[2],format.blue)|pack_channel(c[3],format.alpha);
    for(unsigned j=0;j<bytes;++j)check(output[i*bytes+j]==U8(golden>>(8*j)),"Original pixel conversion differs from scalar golden");++comparisons;
   }
  }
 }
 const std::vector<U8> white={255,255,255,255};
 rejects([&]{reign::convert_bitmap_pixels(0,1,32,white,reign::rgba8_format);});
 rejects([&]{reign::convert_bitmap_pixels(1,1,24,white,reign::rgba8_format);});
 rejects([&]{reign::convert_bitmap_pixels(1,1,8,{0},reign::rgba8_format);});
 rejects([&]{reign::convert_bitmap_pixels(1,1,32,white,{32,5,0xff00,0xff0000,0xff000000});});
 rejects([&]{reign::convert_bitmap_pixels(1,1,32,white,{16,0xf800,0xf800,31,0});});
 rejects([&]{reign::convert_bitmap_pixels(1,1,32,white,{24,255,0xff00,0xff0000,0});});
 std::cout<<"Original Pix/Bitmap::CopyLine: "<<comparisons<<" golden pixel comparisons PASS (including original 1555 alpha=128 rule); invalid formats reject\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
