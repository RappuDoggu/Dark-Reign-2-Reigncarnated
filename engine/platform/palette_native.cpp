#include "palette_native.h"
#include <cstdint>
#include <cstring>
namespace NativeEngine {
bool DecodeRiffPalette(const void *memory,std::size_t size,std::array<PALETTEENTRY,256> &palette){
 if(!memory||size<12)return false;
 const auto *bytes=static_cast<const std::uint8_t *>(memory);
 auto word=[&](std::size_t at){return unsigned(bytes[at])|(unsigned(bytes[at+1])<<8);};
 auto dword=[&](std::size_t at){return std::uint32_t(word(at))|(std::uint32_t(word(at+2))<<16);};
 if(std::memcmp(bytes,"RIFF",4)||std::memcmp(bytes+8,"PAL ",4))return false;
 const std::uint64_t end=std::uint64_t(dword(4))+8;
 if(end>size||end<12)return false;
 bool found=false;std::array<PALETTEENTRY,256> parsed{};
 for(std::uint64_t at=12;at<end;){
  if(end-at<8)return false;
  const auto length=dword(std::size_t(at+4));const auto payload=at+8;
  if(length>end-payload)return false;
  if(!std::memcmp(bytes+at,"data",4)){
   if(found||length<4||word(std::size_t(payload))!=0x300)return false;
   const unsigned count=word(std::size_t(payload+2));
   if(!count||count>256||std::uint64_t(count)*4+4>length)return false;
   std::memcpy(parsed.data(),bytes+payload+4,std::size_t(count)*4);found=true;
  }
  at=payload+length+(length&1);
  if(at>end)return false;
 }
 if(found)palette=parsed;
 return found;
}
}
