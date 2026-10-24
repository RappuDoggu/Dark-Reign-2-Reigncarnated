#pragma once
// Isolated dependencies for the original TBuf/StrBuf headers and implementation.
#include "reign/legacy_types.hpp"
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <stdexcept>
using Bool = std::int32_t;
constexpr Bool TRUE = 1, FALSE = 0;
constexpr U32 U8_MAX = 255;
#define FASTCALL
#define CDECL
#define ASSERT(x) if (!(x)) throw std::runtime_error("Original tokenizer assertion: " #x);
namespace reign {
[[noreturn]] inline void config_error(const char *format, ...) {
 char out[2048]; va_list args; va_start(args, format);
 std::vsnprintf(out, sizeof(out), format, args); va_end(args);
 throw std::runtime_error(out);
}
}
#define ERR_CONFIG(args) reign::config_error args
namespace Utils {
inline char *Strcpy(char *out, const char *in) { return std::strcpy(out, in); }
inline char *Strmcpy(char *out, const char *in, U32 capacity) {
 if (!capacity) throw std::runtime_error("Zero string capacity");
 std::strncpy(out, in, capacity); out[capacity - 1] = 0; return out;
}
inline int Strcmp(const char *a, const char *b) { return std::strcmp(a, b); }
inline void *Memset(void *p, int c, U32 count) { return std::memset(p, c, count); }
}
#include "strbuf.h"
