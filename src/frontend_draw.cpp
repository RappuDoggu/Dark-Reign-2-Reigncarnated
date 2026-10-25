#include "reign/frontend_draw.hpp"
#include <deque>
namespace {
struct PendingPrimitive {std::vector<VertexTL> vertices;reign::FrontendBatch batch;};
struct PrimitiveCollection {std::deque<PendingPrimitive> primitives;};
thread_local PrimitiveCollection *active=nullptr;
}
namespace IFace {
VertexTL *GetVertHelper(U32 vertices,U32 indices,const TextureInfo *texture){
 if(!active)throw std::runtime_error("Frontend drawing not bound");
 if(!((vertices==3&&indices==3)||(vertices==4&&indices==6)))throw std::runtime_error("Unsupported frontend draw allocation");
 active->primitives.emplace_back();auto &p=active->primitives.back();p.vertices.resize(vertices);
 p.batch.indices=indices==3?std::vector<std::uint16_t>{0,1,2}:std::vector<std::uint16_t>{0,1,2,0,2,3};
 if(texture){p.batch.texture=texture->texture;p.batch.filtered=texture->filter;p.batch.clamped=texture->texMode==TextureInfo::TM_CENTRED;}
 return p.vertices.data();
}
}
namespace reign {
void FrontendDraw::run(const std::function<void()> &draw,const Sink &sink){
 if(!sink)throw std::invalid_argument("Submission callback required");
 run_batches(draw,[&](const FrontendBatch &b){if(b.texture)throw std::runtime_error("Textured batch requires texture-aware consumer");sink(b.vertices,b.indices);});
}
void FrontendDraw::run_batches(const std::function<void()> &draw,const std::function<void(const FrontendBatch &)> &sink){
 if(!draw||!sink)throw std::invalid_argument("Drawing and submission callbacks required");
 PrimitiveCollection state;auto *previous=active;active=&state;
 struct Restore {PrimitiveCollection *previous;~Restore(){active=previous;}} restore{previous};
 draw();active=nullptr;
 for(auto &p:state.primitives){
  p.batch.vertices.reserve(p.vertices.size());
  for(const auto &v:p.vertices){LegacyTL out{};out.x=v.vv.x;out.y=v.vv.y;out.z=v.vv.z;out.rhw=v.rhw;
   out.diffuse=v.diffuse;out.specular=v.specular;out.u=v.u;out.v=v.v;p.batch.vertices.push_back(out);}
  sink(p.batch);
 }
}
}
