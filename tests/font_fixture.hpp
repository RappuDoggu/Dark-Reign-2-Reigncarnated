#pragma once
#include <cstdint>
#include <vector>
inline std::vector<std::uint8_t> font_fixture(unsigned version=2,unsigned count=2,unsigned size=2){
 using U8=std::uint8_t;std::vector<U8> b=version==1?std::vector<U8>{'F','O','N','T',U8(count)}:std::vector<U8>{'B','O','R','K',U8(count),U8(count>>8)};
 b.insert(b.end(),{U8(size+2),U8(size+1),1});
 for(unsigned i=0;i<count;++i){b.push_back(U8(65+i));if(version==2)b.push_back(U8((65+i)>>8));b.insert(b.end(),{U8(size+1),0,0,U8(size),U8(size)});}
 for(unsigned i=0;i<count;++i){b.push_back(U8(size));b.push_back(U8(size));for(unsigned p=0;p<size*size;++p)b.push_back(U8(p%4==0?255:p%4==1?128:p%4==2?64:0));}
 return b;
}
