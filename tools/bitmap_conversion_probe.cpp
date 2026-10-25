#include "reign/archive.hpp"
#include "reign/bitmap_pixels.hpp"
#include "reign/tga.hpp"
#include "reign/bitmap_loader.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
int main(int argc,char **argv){try{
 if(argc!=2)throw std::runtime_error("Usage: bitmap_conversion_probe installation/base.zwp");
 reign::Archive archive(argv[1]);unsigned tested=0,unsupported=0,pics=0;std::uint64_t pixels=0;
 for(const auto &entry:archive.entries()){
  auto name=entry.name;std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return std::tolower(c);});
  if(name.size()<4)continue;
  if(name.substr(name.size()-4)==".pic"){
   auto loaded=reign::load_bitmap_pic(archive.read(entry.name));++pics;pixels+=std::uint64_t(loaded.width)*loaded.height;continue;
  }
  if(name.substr(name.size()-4)!=".tga")continue;
  const auto data=archive.read(entry.name);
  if(data.size()<18)throw std::runtime_error("Truncated TGA header");
  if(data[1]||(data[2]!=2&&data[2]!=10)||(data[16]!=24&&data[16]!=32)){
   auto loaded=reign::load_bitmap_tga(data);
   ++unsupported;pixels+=std::uint64_t(loaded.width)*loaded.height;
   std::cout<<"Extended source format loaded: "<<entry.name<<" type="<<unsigned(data[2])<<" depth="<<unsigned(data[16])<<" width="<<loaded.width<<" height="<<loaded.height<<'\n';continue;
  }
  auto image=reign::decode_tga(data);auto bgra=image.pixels;
  for(std::size_t i=0;i<bgra.size();i+=4)std::swap(bgra[i],bgra[i+2]);
  const auto converted=reign::convert_bitmap_pixels(image.width,image.height,32,bgra,reign::rgba8_format);
  if(converted!=image.pixels||reign::load_bitmap_tga(data).pixels!=image.pixels)throw std::runtime_error("Original CopyLine differs for "+entry.name);
  ++tested;pixels+=std::uint64_t(image.width)*image.height;
 }
 std::cout<<"TGA -> original Pix/CopyLine: independently_compared_images="<<tested<<" pixels="<<pixels<<" extended_formats_loaded="<<unsupported<<" PIC_loaded="<<pics<<" comparison_mismatches=0\n";
 if(!tested)throw std::runtime_error("No supported real TGA files exercised");
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
