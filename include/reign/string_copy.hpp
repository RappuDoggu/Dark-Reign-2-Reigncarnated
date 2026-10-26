#pragma once
#include "reign/legacy_types.hpp"
#include <cstring>
#include <stdexcept>
namespace Utils {
inline char *Strmcpy(char *out, const char *in, U32 capacity) {
 if (!capacity) throw std::runtime_error("Zero string capacity");
 // Original menu substitutions can return the destination itself. Measure
 // before writing and use memmove for self-copy and overlapping slices.
 U32 n = 0; while (n < capacity - 1 && in[n]) ++n;
 std::memmove(out, in, n); std::memset(out + n, 0, capacity - n); return out;
}
}
