#pragma once
#include "dx7/windows.h"
#include <array>
#include <cstddef>
namespace NativeEngine {
// Validate the complete RIFF/PAL container before changing the destination.
bool DecodeRiffPalette(const void *,std::size_t,std::array<PALETTEENTRY,256> &);
}
