#pragma once
#include <cstdint>
// Deliberately scoped to isolated upstream translation units, not a Win32 shim.
using U8 = std::uint8_t;
using U32 = std::uint32_t;
using CH = std::uint16_t;
using F32 = float;
static_assert(sizeof(U32) == 4 && sizeof(CH) == 2 && sizeof(F32) == 4);
inline U32 _rotr(U32 value, int count) noexcept {
  const auto shift = static_cast<unsigned>(count) & 31u;
  return shift ? (value >> shift) | (value << (32u - shift)) : value;
}
