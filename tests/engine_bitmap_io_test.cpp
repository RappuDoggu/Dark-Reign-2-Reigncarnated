#include "palette_native.h"
#include <vector>
#include <cstring>
#include <stdexcept>
#include <iostream>
void check(bool ok){if(!ok)throw std::runtime_error("RIFF palette invariant");}
int main(){try{
 std::vector<unsigned char> riff={'R','I','F','F',24,0,0,0,'P','A','L',' ','d','a','t','a',12,0,0,0,0,3,2,0,10,20,30,0,40,50,60,1};
 std::array<PALETTEENTRY,256> colors{};
 check(NativeEngine::DecodeRiffPalette(riff.data(),riff.size(),colors));
 check(colors[0].peRed==10&&colors[0].peBlue==30&&colors[1].peGreen==50&&colors[1].peFlags==1&&colors[2].peRed==0);
 auto before=colors;
 for(std::size_t n=0;n<riff.size();++n){check(!NativeEngine::DecodeRiffPalette(riff.data(),n,colors));check(!std::memcmp(before.data(),colors.data(),sizeof(colors)));}
 auto broken=riff;broken[21]=2;check(!NativeEngine::DecodeRiffPalette(broken.data(),broken.size(),colors));
 broken=riff;broken[23]=2;check(!NativeEngine::DecodeRiffPalette(broken.data(),broken.size(),colors));
 broken=riff;broken[16]=255;check(!NativeEngine::DecodeRiffPalette(broken.data(),broken.size(),colors));
 // Skip an unknown odd-length chunk, including its alignment byte.
 auto padded=riff;padded.insert(padded.begin()+12,{'J','U','N','K',1,0,0,0,99,0});padded[4]=34;
 check(NativeEngine::DecodeRiffPalette(padded.data(),padded.size(),colors));
 std::cout<<"BMP ABI and bounded RIFF palette decoding passed\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
