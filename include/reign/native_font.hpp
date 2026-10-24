#pragma once
#include "reign/bitmap_surface.hpp"
#include "reign/legacy_gles.hpp"
#include <memory>
#include <string>
namespace reign {
struct FontBatch {
 unsigned atlas=0;bool filtered=false;std::uint32_t flags=0;
 std::vector<LegacyTL> vertices;std::vector<std::uint16_t> indices;
};
struct FontDrawOptions {
 std::int32_t x=0,y=0;PixelRect clip{{0,0},{640,480}};
 std::uint32_t argb=0xffffffff;float alpha_scale=1;
 std::int32_t shadow=0;std::uint8_t shadow_alpha=128;
 unsigned batch_vertices=4096;
};
// Original Font reader/layout/draw and IFace batching; not full FontSys registration.
class NativeFont {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 explicit NativeFont(std::vector<std::uint8_t> file,bool half_texel=true);
 ~NativeFont();
 NativeFont(const NativeFont&)=delete;NativeFont &operator=(const NativeFont&)=delete;
 unsigned version()const;unsigned glyph_count()const;
 unsigned height()const;unsigned width(const std::u16string &)const;
 unsigned atlas_count()const;const RgbaImage &atlas(unsigned)const;
 // Caller owns any GPU handles and can upload again after context recreation.
 void upload(const std::function<void(unsigned,const RgbaImage &)> &)const;
 void draw(const std::u16string &,const FontDrawOptions &,
           const std::function<void(const FontBatch &)> &)const;
};
}
