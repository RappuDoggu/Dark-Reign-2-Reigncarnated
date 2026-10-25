#pragma once
#include "reign/runtime_prelude.hpp"
#include "ntree.h"
#include "tracksys.h"
#include <limits>
template<class T> inline T Min(T a,T b){return std::min(a,b);}
namespace Utils {
inline void *Memcpy(void *d,const void *s,std::size_t n){return std::memcpy(d,s,n);}
inline char *Strncpy(char *d,const char *s,U32 n){return std::strncpy(d,s,n);}
inline const char *Strrchr(const char *s,char c){return std::strrchr(s,c);}
inline char *ItoA(int value,char *out,int radix){if(radix<2||radix>36)throw std::runtime_error("Invalid integer radix");std::uint32_t n=static_cast<std::uint32_t>(value);bool negative=value<0&&radix==10;if(negative)n=0u-n;char buffer[33];unsigned used=0;do{buffer[used++]="0123456789abcdefghijklmnopqrstuvwxyz"[n%radix];n/=radix;}while(n);unsigned at=0;if(negative)out[at++]='-';while(used)out[at++]=buffer[--used];out[at]=0;return out;}
// Windows ACP selection is not ported. ASCII has a locale-independent mapping.
inline const CH *Ansi2Unicode(const char *s){static thread_local CH out[256];std::size_t n=std::strlen(s);if(n>=256)throw std::runtime_error("Variable display string exceeds Unicode buffer");for(std::size_t i=0;i<n;++i){if(static_cast<unsigned char>(s[i])>=128)throw std::runtime_error("Non-ASCII Windows code-page conversion is not ported");out[i]=static_cast<unsigned char>(s[i]);}out[n]=0;return out;}
}
namespace reign::vars_detail {
using GameIdent=StrCrc<64>;
using tracking_detail::TrackSys;using tracking_detail::DTrack;
template<class T>using Reaper=tracking_detail::Reaper<T>;
}
