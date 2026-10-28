#pragma once
#include "defines.h"
#include <cstddef>
#include <ctime>
// Native state is deliberately opaque: no HANDLE or pointer-to-U32 conversion.
struct NativeFileState;
struct NativeFindState;
struct NativeFindData {
  U32 attrib = 0;
  std::time_t time_create = 0, time_access = 0, time_write = 0;
  U32 size = 0;
  char name[261] = {};
};
