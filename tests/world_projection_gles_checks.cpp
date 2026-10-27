#include "reign/world_projection.hpp"
#include "reign/legacy_gles.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
void original_world_gpu_checks(reign::LegacyGles &renderer){
 const std::uint16_t indices[]={0,1,2};
 const std::array<std::array<float,3>,3> points={{{-1,1,2},{1,1,2},{0,-1,2}}};
 auto render=[&](std::array<float,3> position){
  reign::WorldProjection projection(position,{0,0,1},1.57079632679f,1,1,11);
  std::array<reign::LegacyClip,3> vertices{};
  for(unsigned i=0;i<3;++i){auto p=projection.transform(points[i]);vertices[i]={p[0],p[1],p[2],p[3],0xff00ff00,0xff000000,0,0};}
  reign::DrawState state;state.depth_test=true;state.depth_write=true;renderer.clear(32,32,0,0,0);renderer.draw_clip(32,32,vertices.data(),3,indices,3,state);
  std::array<std::uint8_t,32*32*4> pixels{};renderer.read_rgba(32,32,pixels.data());return pixels;
 };
 auto centered=render({0,0,0});auto shifted=render({1,0,0});
 auto green=[](auto &p,unsigned x,unsigned y){auto i=((31-y)*32+x)*4;return p[i]==0&&p[i+1]==255&&p[i+2]==0;};
 if(!green(centered,16,12)||green(centered,16,24)||!green(shifted,8,12)||green(shifted,20,12))throw std::runtime_error("Original camera transform to GLES orientation/translation");
 std::cout<<"Original camera matrix to homogeneous GLES: PASS\n";
}
