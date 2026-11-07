#pragma once
#include <array>
#include <cstdint>
#include <vector>
namespace reign {
// Top-down, tightly packed source rows; no color/alpha conversion yet.
struct TgaSource {
 std::uint32_t width,height,depth;
 std::vector<std::uint8_t> rows;
 std::vector<std::array<std::uint8_t,4>> palette;
};
// Supports indexed8, ARGB1555, BGR24 and BGRA32, raw or RLE.
// Grayscale and interleaved images are deliberately rejected.
TgaSource decode_tga_source(const std::vector<std::uint8_t> &);
}
