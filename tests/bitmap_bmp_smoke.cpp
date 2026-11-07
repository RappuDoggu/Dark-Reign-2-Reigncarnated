#include "reign/bitmap_loader.hpp"
#include "bmp_fixture.hpp"
#include <iostream>
int main(){try{
 unsigned checks=0;auto check=[&](bool ok){++checks;if(!ok)throw std::runtime_error("BMP check "+std::to_string(checks));};
 auto reject=[&](auto bytes){bool threw=false;try{reign::load_bitmap_bmp(bytes);}catch(const std::exception &){threw=true;}check(threw);};
 for(bool top:{false,true}){
  auto b=bmp_fixture(24,1,2,top);const unsigned first=top?54:58,second=top?58:54;b[first+2]=255;b[second+1]=255;b[57]=b[61]=123;
  check(reign::load_bitmap_bmp(b).pixels==std::vector<std::uint8_t>({255,0,0,255,0,255,0,255}));
  for(std::size_t n=0;n<b.size();++n)reject(std::vector<std::uint8_t>(b.begin(),b.begin()+n));
 }
 auto b=bmp_fixture(8);b[62]=1;for(std::size_t n=0;n<b.size();++n)reject(std::vector<std::uint8_t>(b.begin(),b.begin()+n));check(reign::load_bitmap_bmp(b).pixels==std::vector<std::uint8_t>({255,0,0,255}));b[61]=0;check(reign::load_bitmap_bmp(b).pixels[3]==255);b[62]=2;reject(b);
 b=bmp_fixture(8);b.insert(b.begin()+62,254*4,0);bmp_word(b,2,b.size());bmp_word(b,10,1078);bmp_word(b,46,0);b[1078]=1;check(reign::load_bitmap_bmp(b).pixels==std::vector<std::uint8_t>({255,0,0,255}));
 reject(bmp_fixture(24,0,1));reject(bmp_fixture(24,1,0));
 b=bmp_fixture(16);b[54]=0;b[55]=252;check(reign::load_bitmap_bmp(b).pixels==std::vector<std::uint8_t>({248,0,0,128}));
 b=bmp_fixture(32);b[54]=17;b[55]=33;b[56]=65;b[57]=0;check(reign::load_bitmap_bmp(b).pixels==std::vector<std::uint8_t>({65,33,17,0}));b[57]=127;check(reign::load_bitmap_bmp(b).pixels[3]==127);
 for(unsigned field:{2u,10u,14u,18u,22u,30u}){auto bad=b;bmp_word(bad,field,field==22?0x80000000u:0xffffffffu);reject(bad);}
 auto bad=b;bad[26]=2;reject(bad);bad=b;bad[28]=4;reject(bad);bad=b;bmp_word(bad,30,3);reject(bad); // Explicitly reject unported BI_BITFIELDS.
 bad=bmp_fixture(8);bmp_word(bad,46,257);reject(bad);bad=bmp_fixture(8);bmp_word(bad,10,54);reject(bad);
 b=bmp_fixture(24);b[56]=255;bmp_word(b,2,0);check(reign::load_bitmap_bmp(b).pixels[0]==255);
 std::cout<<"BMP framing -> original CopyLine padding/origin/palette/alpha/bounds: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
