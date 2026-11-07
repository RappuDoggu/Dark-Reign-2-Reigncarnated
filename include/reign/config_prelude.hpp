#pragma once
#include "reign/tokenizer_prelude.hpp"
#include <cctype>
#include <cstdlib>
#include <algorithm>
#include <limits>
#include <exception>
#include <cfenv>
#undef ERR_CONFIG
#define ERR_CONFIG(args) reign::config_error args;
#define VALIDATE(x) ASSERT(x)
constexpr U32 U16_MAX = 65535;
using S32 = std::int32_t;
using U16 = std::uint16_t;
#define ERR_FATAL(args) reign::config_error args;
#define NORETURN __attribute__((noreturn))
#define LOGDEC
#define LOGDEF(a,b)
namespace reign {
inline void engine_log(const char *format,...) {
 char out[2048];va_list args;va_start(args,format);std::vsnprintf(out,sizeof(out),format,args);va_end(args);
 std::fprintf(stderr,"[engine] %s\n",out);
}
}
#define LOG_WARN(args) reign::engine_log args;
#define LOG_ERR(args) reign::engine_log args;
#define LOG_DIAG(args) reign::engine_log args;
namespace Utils {
inline char *Strdup(const char *s) { auto n=std::strlen(s)+1; auto *p=new char[n];std::memcpy(p,s,n);return p; }
inline U32 Strlen(const char *s) { return static_cast<U32>(std::strlen(s)); }
inline char *Strcat(char *d,const char *s) { return std::strcat(d,s); }
inline const char *Strchr(const char *s,char c) { return std::strchr(s,c); }
inline S32 AtoI(const char *s) { return std::atoi(s); }
inline F32 AtoF(const char *s) { std::fesetround(FE_TONEAREST); return static_cast<F32>(std::atof(s)); }
inline int Stricmp(const char *a,const char *b) { for(;;++a,++b) { auto x=std::tolower(static_cast<unsigned char>(*a)),y=std::tolower(static_cast<unsigned char>(*b));if(x!=y||!x||!y)return x-y; } }
inline int Sprintf(char *out,U32 size,const char *format,...) { va_list args;va_start(args,format);int n=std::vsnprintf(out,size,format,args);va_end(args);return n; }
}
#include "crc.h"
#include "nlist.h"
#include "tree.h"
#include "list.h"
#include "strcrc.h"
using PathIdent = StrCrc<270>;
#include "reign/engine_file.hpp"
