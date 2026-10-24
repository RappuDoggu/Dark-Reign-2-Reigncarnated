#pragma once
#include "reign/legacy_gles.hpp"
#include <array>
#include <cstdint>
#include <functional>
namespace reign {
// Narrow D3D7 numeric state boundary, not a COM or DirectDraw implementation.
class TextureView {
 unsigned handle_;
public:
 explicit TextureView(unsigned handle):handle_(handle){}
 unsigned GetTexture() const {return handle_;}
};
class VidDevice {
 unsigned texture_=0;
 std::array<std::uint32_t,137> render_{};
 using Sink=std::function<void(const LegacyTL *,std::size_t,const std::uint16_t *,std::size_t,const DrawState &)>;
 Sink sink_;
 std::array<std::uint32_t,13> stage_{};
public:
 VidDevice();
 void set_draw_sink(Sink sink);
 int DrawIndexedPrimitive(std::uint32_t primitive,std::uint32_t fvf,void *vertices,
   std::uint32_t count,std::uint16_t *indices,std::uint32_t index_count,std::uint32_t flags);
 int SetRenderState(std::uint32_t key,std::uint32_t value);
 int GetRenderState(std::uint32_t key,std::uint32_t *value) const;
 int SetTextureStageState(std::uint32_t stage,std::uint32_t key,std::uint32_t value);
 int SetTexture(std::uint32_t stage,unsigned texture);
 DrawState draw_state() const;
};
}
