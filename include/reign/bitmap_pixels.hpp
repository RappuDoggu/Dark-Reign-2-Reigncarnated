#pragma once
#include <array>
#include <cstdint>
#include <vector>
namespace reign {
struct PixelFormat {
 std::uint32_t depth,red,green,blue,alpha;
};
constexpr PixelFormat rgba8_format{32,0xff,0xff00,0xff0000,0xff000000};
constexpr PixelFormat rgb565_format{16,0xf800,0x7e0,0x1f,0};
// Source rows are palette indices, TGA-style ARGB1555, BGR24 or BGRA32.
// Returns tightly packed destination rows, retaining original Bitmap::CopyLine rules.
std::vector<std::uint8_t> convert_bitmap_pixels(unsigned width,unsigned height,unsigned source_depth,
 const std::vector<std::uint8_t> &source,PixelFormat destination,
 const std::vector<std::array<std::uint8_t,4>> &palette_rgba={});
}
