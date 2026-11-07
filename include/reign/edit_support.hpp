#pragma once
#include "reign/listbox_support.hpp"
constexpr S32 DIK_BACK=0x0E,DIK_INSERT=0xD2,DIK_DELETE=0xD3,DIK_HOME=0xC7,DIK_END=0xCF,DIK_NUMPADENTER=0x9C,DIK_C=0x2E,DIK_X=0x2D,DIK_V=0x2F,DIK_A=0x1E;
namespace Utils {
inline int Unicode2Ansi(char *out,U32 capacity,const CH *text){
 if(!out||!text||!capacity)throw std::invalid_argument("Invalid edit text buffer");
 U32 n=Strlen(text);if(n>=capacity)throw std::length_error("Edit text too long");
 for(U32 i=0;i<n;++i)if(text[i]>127)throw std::runtime_error("Non-ASCII edit input needs installation encoding conversion");
 for(U32 i=0;i<n;++i)out[i]=char(text[i]);out[n]=0;return int(n);
}
}
