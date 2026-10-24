#include "reign/bitmap_pixels.hpp"
#include "pixel_slice.hpp"
namespace reign {
namespace {
void validate(PixelFormat f){
 if(f.depth!=16&&f.depth!=24&&f.depth!=32)throw std::runtime_error("Unsupported destination pixel depth");
 const U32 masks[]={f.red,f.green,f.blue,f.alpha};U32 used=0;
 for(auto mask:masks){
  if(mask&used)throw std::runtime_error("Overlapping pixel masks");used|=mask;
  if(f.depth<32&&(mask>>f.depth))throw std::runtime_error("Pixel mask exceeds depth");
  if(!mask)continue;while(!(mask&1))mask>>=1;
  if(mask>255||(mask&(mask+1)))throw std::runtime_error("Pixel channels must be contiguous and at most 8 bits");
 }
 if(!f.red||!f.green||!f.blue)throw std::runtime_error("RGB channels required");
 if(f.depth==24&&(f.red!=0xff0000||f.green!=0xff00||f.blue!=0xff||f.alpha))throw std::runtime_error("Original 24-bit CopyLine requires BGR888");
}
}
std::vector<std::uint8_t> convert_bitmap_pixels(unsigned width,unsigned height,unsigned source_depth,const std::vector<std::uint8_t> &source,PixelFormat destination,const std::vector<std::array<std::uint8_t,4>> &palette){
 validate(destination);
 if(!width||!height||width>16384||height>16384||(source_depth!=8&&source_depth!=16&&source_depth!=24&&source_depth!=32))throw std::runtime_error("Unsupported bitmap dimensions/depth");
 const std::uint64_t count=std::uint64_t(width)*height,input_size=count*(source_depth/8),output_size=count*(destination.depth/8);
 if(input_size!=source.size()||input_size>128*1024*1024||output_size>128*1024*1024)throw std::runtime_error("Bitmap pixel buffer size invalid");
 if(source_depth==8&&palette.size()!=256)throw std::runtime_error("Indexed source requires a 256-entry palette");
 std::array<RGBQUAD,256> pal{};for(unsigned i=0;i<palette.size()&&i<256;++i)pal[i]={palette[i][2],palette[i][1],palette[i][0],palette[i][3]};
 Pix pix;DDPIXELFORMAT format;format.dwFlags=DDPF_RGB|(destination.alpha?DDPF_ALPHAPIXELS:0);format.dwRGBBitCount=destination.depth;format.dwRBitMask=destination.red;format.dwGBitMask=destination.green;format.dwBBitMask=destination.blue;format.dwRGBAlphaBitMask=destination.alpha;pix.SetPixFmt(format);
 Bitmap bitmap;bitmap.bmpDepth=destination.depth;bitmap.pixForm=&pix;
 std::vector<U8> result(static_cast<std::size_t>(output_size));
 for(unsigned row=0;row<height;++row)bitmap.CopyLine(const_cast<U8 *>(source.data())+row*width*(source_depth/8),result.data()+row*width*(destination.depth/8),width,source_depth,source_depth==8?pal.data():nullptr);
 return result;
}
}
