#pragma once
#include "reign/tga.hpp"
#include <vector>
namespace reign {
// Native format framing followed by original Pix/Bitmap::CopyLine conversion.
// This is not the original Bitmap registry, mipmap generation or ReadTGA method.
RgbaImage load_bitmap_bmp(const std::vector<std::uint8_t> &);
RgbaImage load_bitmap_pic(const std::vector<std::uint8_t> &);
RgbaImage load_bitmap_tga(const std::vector<std::uint8_t> &);
}
