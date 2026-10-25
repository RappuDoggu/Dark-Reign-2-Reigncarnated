#include "reign/frontend_draw.hpp"
#include "reign/frontend_bitmap.hpp"
#include <array>
#include <cmath>
#include <iostream>
void original_frontend_gpu_checks(reign::LegacyGles &renderer){
 std::array<std::uint8_t,32*32*4> pixels{};
 auto pixel=[&](unsigned x,unsigned y,int r,int g,int b){renderer.read_rgba(32,32,pixels.data());auto at=((31-y)*32+x)*4;
  return std::abs(int(pixels[at])-r)<=2&&std::abs(int(pixels[at+1])-g)<=2&&std::abs(int(pixels[at+2])-b)<=2;};
 auto check=[](bool b,const char *name){if(!b)throw std::runtime_error(name);std::cout<<name<<": PASS\n";};
 unsigned texture=0;
 auto sink=[&](const reign::FrontendBatch &b){reign::DrawState state;state.blend=reign::Blend::alpha;
  if(b.filtered)throw std::runtime_error("Filtered UI submission not configured in this test");
  if(b.texture){state.texture=texture;state.texture_op=reign::TextureOp::modulate_rgba;state.address=b.clamped?reign::TextureAddress::clamp:reign::TextureAddress::wrap;}
  renderer.draw(32,32,b.vertices.data(),b.vertices.size(),b.indices.data(),b.indices.size(),state);};
 ClipRect rect;rect.p0={4,4};rect.p1={20,20};
 renderer.clear(32,32,0,0,0);reign::FrontendDraw::run_batches([&]{IFace::RenderRectangle(rect,Color(U32(0xffff0000)),nullptr,1);},sink);
 check(pixel(8,8,255,0,0)&&pixel(2,2,0,0,0),"Original UI rectangle to GLES");
 renderer.clear(32,32,0,0,0);reign::FrontendDraw::run_batches([&]{IFace::RenderRectangle(rect,Color(U32(0xffff0000)),nullptr,.5f);},sink);
 check(pixel(8,8,127,0,0),"Original UI alpha scaling to GLES");
 auto surface=std::make_shared<reign::BitmapSurface>([](const auto &){});surface->Create(2,2,true);surface->Clear(reign::BitmapSurface::MakeRGBA(0,255,0,255));Bitmap bitmap(surface);
 const auto &image=surface->pixels();texture=renderer.texture_rgba(image.width,image.height,image.pixels.data());
 try{
  TextureInfo info;info.texture=&bitmap;info.texMode=TextureInfo::TM_TILED;
  renderer.clear(32,32,0,0,0);reign::FrontendDraw::run_batches([&]{IFace::RenderRectangle(rect,Color(U32(0xffffffff)),&info,1);},sink);
  check(pixel(8,8,0,255,0),"Original UI tiled texture to GLES");
 }catch(...){renderer.delete_texture(texture);throw;}
 renderer.delete_texture(texture);
 renderer.clear(32,32,0,0,0);reign::FrontendDraw::run_batches([&]{IFace::RenderGradient(rect,Color(U32(0xffff0000)),Color(U32(0xff0000ff)),TRUE,nullptr,1);},sink);
 renderer.read_rgba(32,32,pixels.data());auto top=((31-6)*32+8)*4,bottom=((31-17)*32+8)*4;
 check(pixels[top]>pixels[bottom]&&pixels[top+2]<pixels[bottom+2],"Original UI gradient direction to GLES");
}
