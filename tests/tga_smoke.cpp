#include "reign/tga.hpp"
#include <iostream>
int main(){
 int fail=0;auto check=[&](bool b){if(!b)++fail;};
 std::vector<std::uint8_t> bytes(18);bytes[2]=2;bytes[12]=2;bytes[14]=1;bytes[16]=24;bytes[17]=32;
 bytes.insert(bytes.end(),{0,0,255,0,255,0});auto img=reign::decode_tga(bytes);
 check(img.width==2&&img.height==1&&img.pixels==std::vector<std::uint8_t>({255,0,0,255,0,255,0,255}));
 bytes[17]=48;auto flip=reign::decode_tga(bytes);check(flip.pixels[0]==0&&flip.pixels[1]==255&&flip.pixels[4]==255);
 bytes[17]=32;bytes[2]=10;bytes.resize(18);bytes.insert(bytes.end(),{129,255,0,0});auto rle=reign::decode_tga(bytes);check(rle.pixels[2]==255&&rle.pixels[6]==255);
 auto rejects=[&](auto b){try{reign::decode_tga(b);++fail;}catch(const std::exception &){}};
 for(std::size_t n=0;n<bytes.size();++n)rejects(std::vector<std::uint8_t>(bytes.begin(),bytes.begin()+n));
 auto overflow=bytes;overflow[18]=130;rejects(overflow);auto color_map=bytes;color_map[1]=1;rejects(color_map);
 std::cout<<"TGA format/origin/RLE/bounds: "<<(fail?"FAIL":"PASS")<<" ("<<fail<<" failures)\n";return fail?1:0;
}
