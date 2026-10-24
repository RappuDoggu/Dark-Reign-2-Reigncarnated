#pragma once
#include <cstdint>
#include <vector>
namespace reign {
struct RgbaImage { std::uint32_t width,height;std::vector<std::uint8_t> pixels; };
// Standard true-color TGA bridge for the native renderer, not a replacement for Bitmap.
RgbaImage decode_tga(const std::vector<std::uint8_t> &);
}
