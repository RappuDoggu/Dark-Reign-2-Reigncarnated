#pragma once
#include <vector>
#include <cstdint>
inline void bmp_word(std::vector<std::uint8_t> &b,unsigned p,std::uint32_t value){for(unsigned i=0;i<4;++i)b[p+i]=std::uint8_t(value>>(i*8));}
inline std::vector<std::uint8_t> bmp_fixture(unsigned depth=24,unsigned width=1,unsigned height=1,bool top=false){
 const unsigned offset=54+(depth==8?8:0),stride=((width*(depth/8))+3)&~3u;
 std::vector<std::uint8_t>b(offset+stride*height);b[0]='B';b[1]='M';bmp_word(b,2,b.size());bmp_word(b,10,offset);bmp_word(b,14,40);bmp_word(b,18,width);bmp_word(b,22,top?0u-height:height);b[26]=1;b[28]=depth;
 if(depth==8){bmp_word(b,46,2);b[60]=255;}return b;
}
