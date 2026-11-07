#include "reign/archive.hpp"
#include "reign/native_font.hpp"
#include <algorithm>
#include <iostream>
int main(int argc,char **argv){try{
 if(argc!=2)throw std::runtime_error("Usage: font_probe installation/base.zwp");
 reign::Archive archive(argv[1]);unsigned fonts=0,atlases=0,glyphs=0;std::uint64_t vertices=0;
 for(const auto &entry:archive.entries()){
  auto name=entry.name;std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return std::tolower(c);});if(name.size()<4||name.substr(name.size()-4)!=".fff")continue;
  reign::NativeFont font(archive.read(entry.name));++fonts;atlases+=font.atlas_count();glyphs+=font.glyph_count();
  font.draw(u"Dark Reign II 0123456789",{},[&](const auto &batch){vertices+=batch.vertices.size();});
  std::cout<<"FONT "<<entry.name<<" version="<<font.version()<<" height="<<font.height()<<" glyphs="<<font.glyph_count()<<" atlases="<<font.atlas_count()<<" sample_width="<<font.width(u"Dark Reign II")<<'\n';
 }
 if(!fonts)throw std::runtime_error("No original FFF fonts exercised");
 std::cout<<"SUMMARY original_fonts="<<fonts<<" glyphs="<<glyphs<<" atlases="<<atlases<<" sample_vertices="<<vertices<<" failures=0\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
