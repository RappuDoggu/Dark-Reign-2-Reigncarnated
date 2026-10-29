#pragma once
#include "defines.h"
#include <cstdarg>
namespace NativeEngine {
// Legacy engine strings are UTF-16. ANSI boundaries use the English retail
// installation's Windows-1252 encoding, independently of the host locale.
S32 VFormat16(CH *out,S32 capacity,const CH *format,va_list args);
}
#include <array>
namespace NativeEngine {
template<std::size_t N> constexpr std::array<CH,N> Literal16(const char16_t (&source)[N]) {
 std::array<CH,N> result{};for(std::size_t i=0;i<N;++i)result[i]=CH(source[i]);return result;
}
}
// Each expansion owns immutable UTF-16 storage, with no wchar_t aliasing.
#define REIGN_TEXT16(text) ([]() { static constexpr auto value=NativeEngine::Literal16(u##text); return value.data(); }())
