#include "reign/bitmap_loader.hpp"
#include "reign/bitmap_pixels.hpp"
#include <algorithm>
#include <stdexcept>
namespace reign {
RgbaImage load_bitmap_bmp(const std::vector<std::uint8_t> &b){
 if(b.size()<54||b[0]!='B'||b[1]!='M')throw std::runtime_error("Invalid/truncated BMP header");
 auto u16=[&](unsigned n){return unsigned(b[n])|(unsigned(b[n+1])<<8);};
 auto u32=[&](unsigned n){return std::uint32_t(b[n])|(std::uint32_t(b[n+1])<<8)|(std::uint32_t(b[n+2])<<16)|(std::uint32_t(b[n+3])<<24);};
 auto signed32=[](std::uint32_t x)->std::int64_t{return x<=0x7fffffffu?std::int64_t(x):std::int64_t(x)-0x100000000ll;};
 const auto width=signed32(u32(18)),signed_height=signed32(u32(22));
 const auto height=signed_height<0?-signed_height:signed_height;const unsigned depth=u16(28);
 if(u32(14)!=40||u16(26)!=1||u32(30)!=0||(depth!=8&&depth!=16&&depth!=24&&depth!=32))throw std::runtime_error("Unsupported BMP DIB/planes/compression/depth");
 if(width<=0||height<=0||width>4096||height>4096)throw std::runtime_error("Invalid BMP dimensions");
 const std::size_t step=depth/8,row=std::size_t(width)*step,stride=(row+3)&~std::size_t(3),offset=u32(10),declared=u32(2);
 const auto end=offset+stride*std::size_t(height);
 if(offset<54||offset>b.size()||end>b.size()||(declared&&(declared>b.size()||end>declared)))throw std::runtime_error("BMP pixel extent exceeds file");
 std::vector<std::array<std::uint8_t,4>> palette;unsigned colors=0;
 if(depth==8){
  colors=u32(46);if(!colors)colors=256;
  if(colors>256||54+std::size_t(colors)*4>offset)throw std::runtime_error("Invalid/truncated BMP palette");
  palette.resize(256);
  for(unsigned i=0;i<colors;++i){auto p=54+i*4;palette[i]={b[p+2],b[p+1],b[p],255};}
 }
 std::vector<std::uint8_t> rows(row*std::size_t(height));
 for(std::size_t y=0;y<std::size_t(height);++y){
  const auto src=offset+(signed_height<0?y:std::size_t(height)-1-y)*stride;
  if(depth==8)for(std::size_t x=0;x<std::size_t(width);++x)if(b[src+x]>=colors)throw std::runtime_error("BMP palette index outside declared table");
  std::copy_n(b.data()+src,row,rows.data()+y*row);
 }
 return {unsigned(width),unsigned(height),convert_bitmap_pixels(unsigned(width),unsigned(height),depth,rows,rgba8_format,palette)};
}
}
