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
enum class TextureOp { diffuse, modulate_rgba, modulate_rgb_texture_alpha, decal };
enum class Blend { opaque, alpha, additive };
struct DrawState {
 TextureOp texture_op=TextureOp::diffuse;
 Blend blend=Blend::opaque;
 bool depth_test=false,depth_write=false,fog=false,alpha_test=false,specular=false;
 float alpha_reference=0;
 float fog_rgb[3]={0,0,0};
 unsigned texture=0;
};
class LegacyGles {
 struct Impl;std::unique_ptr<Impl> impl_;
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
 // Initial supported Vid boundary: indexed triangle lists of pre-transformed/lit vertices.
 // Other FVF layouts/stages are deliberately not silently accepted.
 void draw(unsigned width,unsigned height,const LegacyTL *,std::size_t vertices,
           const std::uint16_t *,std::size_t indices,const DrawState &);
 void read_rgba(unsigned width,unsigned height,std::uint8_t *out);
};
}
