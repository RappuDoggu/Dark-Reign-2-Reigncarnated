#pragma once
#include "reign/frontend_prelude.hpp"
#include "control_stdload.hpp"
#include "control_input.hpp"
#include "iclistbox.h"
constexpr S32 DIK_UP=0xC8,DIK_LEFT=0xCB,DIK_DOWN=0xD0,DIK_RIGHT=0xCD;
namespace Utils {
inline CH *Strndup(const CH *text,U32 count){if(!text)throw std::invalid_argument("Missing text");auto n=std::min(Strlen(text),count);auto *out=new CH[std::size_t(n)+1];std::copy_n(text,n,out);out[n]=0;return out;}
inline S32 Stricmp(const CH *a,const CH *b){if(!a||!b)throw std::invalid_argument("Missing comparison text");for(;;++a,++b){if(*a>127||*b>127)throw std::runtime_error("Unicode list sorting needs Windows-compatible case mapping");auto x=std::tolower(*a),y=std::tolower(*b);if(x!=y||!x)return x-y;}}
}
namespace StdLoad {
inline Color &TypeColor(FScope *s,Color &c){U32 r=TypeU32(s),g=TypeU32(s),b=TypeU32(s),a=TypeU32(s,255);c.SetInline(r,g,b,a);return c;}
}
