#pragma once
#include "reign/tga.hpp"
#include <functional>
namespace reign {
struct PixelRect {
 struct Point {std::int32_t x=0,y=0;};Point p0,p1;
 std::int32_t Width()const{return p1.x-p0.x;}
 std::int32_t Height()const{return p1.y-p0.y;}
};
// CPU-backed RGBA8 storage. Publication requires a real caller-supplied consumer;
// it does not claim to create a DirectDraw surface or initialize the GPU.
class BitmapSurface {
 RgbaImage image_{};PixelRect clip_{};bool locked_=false,half_texel_;
 std::function<void(const RgbaImage &)> publish_;
 void require_created()const;
public:
 explicit BitmapSurface(std::function<void(const RgbaImage &)> publish,bool half_texel=true);
 bool Create(std::int32_t width,std::int32_t height,bool alpha);
 void Clear(std::uint32_t rgba);
 void *Lock();void UnLock();
 void PutPixel(std::int32_t x,std::int32_t y,std::uint32_t rgba,const PixelRect *);
 static std::uint32_t MakeRGBA(unsigned r,unsigned g,unsigned b,unsigned a=255);
 void Publish();
 std::int32_t Width()const{return image_.width;}std::int32_t Height()const{return image_.height;}
 const PixelRect &GetClipRect()const{return clip_;}
 float UVShiftWidth()const;float UVShiftHeight()const;
 const RgbaImage &pixels()const;
};
}
