#include "reign/bitmap_loader.hpp"
#include "reign/bitmap_pixels.hpp"
#include "reign/tga_source.hpp"
namespace reign {
RgbaImage load_bitmap_tga(const std::vector<std::uint8_t> &bytes){
 auto source=decode_tga_source(bytes);
 return {source.width,source.height,convert_bitmap_pixels(source.width,source.height,source.depth,source.rows,rgba8_format,source.palette)};
}
}
