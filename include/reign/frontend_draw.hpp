#pragma once
#include "reign/frontend_prelude.hpp"
#include "reign/legacy_gles.hpp"
// Native TL interchange, with the member spelling used by original UI code.
// This is not a cast to a vendor D3D object.
struct VertexTL {
 struct {F32 x,y,z;} vv;
 F32 rhw;U32 diffuse,specular;F32 u,v;
};
static_assert(sizeof(VertexTL)==32);
namespace IFace {
VertexTL *GetVertHelper(U32,U32,const TextureInfo *);
void RenderTriangle(const Point<S32> *,Color,F32,VertexTL *);
}
namespace reign {
class FrontendDraw {
public:
 using Sink=std::function<void(const std::vector<LegacyTL> &,const std::vector<std::uint16_t> &)>;
 // Runs original UI drawing with native storage, then submits completed batches.
 // Currently supports the original untextured RenderTriangle path only.
 // Submission failure may leave earlier batches delivered; callers must discard
 // the failed frame. Nested contexts restore state and do not share buffers.
 static void run(const std::function<void()> &,const Sink &);
};
}
