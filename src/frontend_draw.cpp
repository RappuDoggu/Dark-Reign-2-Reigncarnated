#include "reign/frontend_draw.hpp"
#include <array>
#include <deque>
namespace {
struct TriangleCollection {std::deque<std::array<VertexTL,3>> triangles;};
thread_local TriangleCollection *active=nullptr;
}
namespace IFace {
VertexTL *GetVertHelper(U32 vertices,U32 indices,const TextureInfo *texture){
 if(!active)throw std::runtime_error("Frontend drawing not bound");
 if(vertices!=3||indices!=3||texture)throw std::runtime_error("Unsupported frontend draw allocation");
 active->triangles.emplace_back();return active->triangles.back().data();
}
}
namespace reign {
void FrontendDraw::run(const std::function<void()> &draw,const Sink &sink){
 if(!draw||!sink)throw std::invalid_argument("Drawing and submission callbacks required");
 TriangleCollection state;auto *previous=active;active=&state;
 struct Restore {TriangleCollection *previous;~Restore(){active=previous;}} restore{previous};
 draw();
 // End collection before invoking external code; callbacks cannot append to the
 // container being iterated. A nested run still gets its own explicit binding.
 active=nullptr;
 const std::vector<std::uint16_t> indices{0,1,2};
 for(const auto &triangle:state.triangles){
  std::vector<LegacyTL> vertices;vertices.reserve(3);
  for(const auto &v:triangle){LegacyTL out{};out.x=v.vv.x;out.y=v.vv.y;out.z=v.vv.z;out.rhw=v.rhw;
   out.diffuse=v.diffuse;out.specular=v.specular;out.u=v.u;out.v=v.v;vertices.push_back(out);}
  sink(vertices,indices);
 }
}
}
