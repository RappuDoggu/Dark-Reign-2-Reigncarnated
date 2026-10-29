#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
namespace reign {
// Matches inspected graphics/Vertex.h VertexTL byte layout; no upstream implementation copied.
struct LegacyTL {
 float x,y,z,rhw;
 std::uint32_t diffuse,specular;
 float u,v;
};
static_assert(sizeof(LegacyTL)==32 && offsetof(LegacyTL,diffuse)==16 && offsetof(LegacyTL,u)==24);
// Original Vid homogeneous coordinates: x/y before viewport mapping (y down),
// Direct3D z in [0,w]. Unlike TL, w may cross zero during clipping.
struct LegacyClip {
 float x,y,z,w;
 std::uint32_t diffuse,specular;
 float u,v;
};
static_assert(sizeof(LegacyClip)==32 && offsetof(LegacyClip,diffuse)==16 && offsetof(LegacyClip,u)==24);
enum class TextureOp { diffuse, modulate_rgba, modulate_rgb_texture_alpha, decal, diffuse_texture_alpha, texture, modulate2x, modulate4x };
enum class Blend { opaque, alpha, additive, custom };
enum class BlendFactor : unsigned { zero=1,one,source_color,inverse_source_color,source_alpha,inverse_source_alpha,destination_alpha,inverse_destination_alpha,destination_color,inverse_destination_color,source_alpha_saturate };
enum class TextureAddress { clamp, wrap, mirror };
struct DrawState {
 TextureOp texture_op=TextureOp::diffuse;
 Blend blend=Blend::opaque;
 BlendFactor source_factor=BlendFactor::source_alpha,destination_factor=BlendFactor::inverse_source_alpha;
 TextureAddress address=TextureAddress::clamp;
 bool filtered=false;
 bool dither=true;
 bool perspective=true;
 bool depth_test=false,depth_write=false,fog=false,alpha_test=false,specular=false;
 float alpha_reference=0;
 float fog_rgb[3]={0,0,0};
 unsigned texture=0;
};
class LegacyGles {
 struct Impl;std::unique_ptr<Impl> impl_;
 void draw_batch(unsigned,unsigned,const void*,std::size_t,const std::uint16_t*,std::size_t,const DrawState&,bool);
public:
 using Resolver=void *(*)(const char *);
 // Requires a current GLES3 context; destruction must occur before releasing that context.
 explicit LegacyGles(Resolver);
 ~LegacyGles();
 LegacyGles(const LegacyGles&)=delete;
 LegacyGles &operator=(const LegacyGles&)=delete;
 unsigned texture_rgba(unsigned width,unsigned height,const std::uint8_t *rgba);
 void delete_texture(unsigned);
 void abandon_context() noexcept;
 void clear(unsigned width,unsigned height,float r,float g,float b,float a=1);
 void clear_buffers(unsigned width,unsigned height,bool color,bool depth,float r,float g,float b,float a,float depth_value);
 // Initial supported Vid boundary: indexed triangle lists of pre-transformed/lit vertices.
 // Other FVF layouts/stages are deliberately not silently accepted.
 void draw(unsigned width,unsigned height,const LegacyTL *,std::size_t vertices,
           const std::uint16_t *,std::size_t indices,const DrawState &);
 void draw_clip(unsigned width,unsigned height,const LegacyClip *,std::size_t vertices,
                const std::uint16_t *,std::size_t indices,const DrawState &);
 // Draw in logical game coordinates, then flip the completed color buffer once.
 void present();
 // Preserve logical aspect ratio within an Android surface (black letterbox).
 void present_to(unsigned surface_width,unsigned surface_height);
 // Both reads return bottom-up display rows; presented read verifies the default target.
 void read_presented_rgba(unsigned width,unsigned height,std::uint8_t *out);
 void read_rgba(unsigned width,unsigned height,std::uint8_t *out);
};
}
