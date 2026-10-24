#pragma once
#include "reign/tokenizer_prelude.hpp"
#include "crc.h"
#include "strcrc.h"
using U16=std::uint16_t;
using S32=std::int32_t;
namespace reign {
inline void pixel_log(const char *fmt,...){va_list args;va_start(args,fmt);std::vfprintf(stderr,fmt,args);va_end(args);std::fputc('\n',stderr);}
}
#define LOG_ERR(args) reign::pixel_log args;
namespace reign::pixel_detail {
using GameIdent=StrCrc<64>;
// Only the fixed-width RGB fields of DDPIXELFORMAT are needed by original Pix.
struct DDPIXELFORMAT {U32 dwSize=32,dwFlags=0,dwFourCC=0,dwRGBBitCount=0,dwRBitMask=0,dwGBitMask=0,dwBBitMask=0,dwRGBAlphaBitMask=0;};
static_assert(sizeof(DDPIXELFORMAT)==32);
constexpr U32 DDPF_ALPHAPIXELS=1,DDPF_ALPHA=2,DDPF_PALETTEINDEXED4=8,DDPF_PALETTEINDEXEDTO8=16,DDPF_PALETTEINDEXED8=32,DDPF_RGB=64,DDPF_PALETTEINDEXED1=0x800,DDPF_PALETTEINDEXED2=0x1000;
struct RGBQUAD {U8 rgbBlue,rgbGreen,rgbRed,rgbReserved;};
struct Bit24Color {U8 b,g,r;};
static_assert(sizeof(RGBQUAD)==4&&sizeof(Bit24Color)==3);
template<std::size_t N>int wsprintf(char (&out)[N],const char *fmt,...){va_list args;va_start(args,fmt);int n=std::vsnprintf(out,N,fmt,args);va_end(args);return n;}

}
