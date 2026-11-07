#include "reign/bitmap_surface.hpp"
#include <stdexcept>
#include <utility>
namespace reign {
BitmapSurface::BitmapSurface(std::function<void(const RgbaImage&)> publish,bool half):half_texel_(half),publish_(std::move(publish)){}
void BitmapSurface::require_created()const{if(image_.pixels.empty())throw std::runtime_error("Bitmap storage not created");}
bool BitmapSurface::Create(std::int32_t w,std::int32_t h,bool alpha){
 if(locked_||!alpha||w<=0||h<=0||w>16384||h>16384||std::uint64_t(w)*h*4>128*1024*1024)throw std::runtime_error("Unsupported native bitmap storage creation");
 RgbaImage replacement{unsigned(w),unsigned(h),std::vector<std::uint8_t>(std::size_t(w)*h*4)};
 image_=std::move(replacement);clip_={{0,0},{w,h}};return true;
}
std::uint32_t BitmapSurface::MakeRGBA(unsigned r,unsigned g,unsigned b,unsigned a){if((r|g|b|a)>255)throw std::runtime_error("Pixel component exceeds byte range");return r|(g<<8)|(b<<16)|(a<<24);}
void BitmapSurface::Clear(std::uint32_t color){require_created();for(std::size_t p=0;p<image_.pixels.size();p+=4)for(unsigned i=0;i<4;++i)image_.pixels[p+i]=std::uint8_t(color>>(i*8));}
void *BitmapSurface::Lock(){require_created();if(locked_)throw std::runtime_error("Bitmap storage already locked");locked_=true;return image_.pixels.data();}
void BitmapSurface::UnLock(){if(!locked_)throw std::runtime_error("Bitmap storage not locked");locked_=false;}
void BitmapSurface::PutPixel(std::int32_t x,std::int32_t y,std::uint32_t color,const PixelRect *clip){
 require_created();if(!locked_)throw std::runtime_error("Pixel write requires locked bitmap");
 if(x<0||y<0||x>=Width()||y>=Height())throw std::runtime_error("Pixel outside native bitmap");
 if(clip&&(x<clip->p0.x||y<clip->p0.y||x>=clip->p1.x||y>=clip->p1.y))return;
 auto p=(std::size_t(y)*image_.width+x)*4;for(unsigned i=0;i<4;++i)image_.pixels[p+i]=std::uint8_t(color>>(i*8));
}
void BitmapSurface::Publish(){require_created();if(locked_||!publish_)throw std::runtime_error("Bitmap publication needs unlocked storage and a consumer");publish_(image_);}
float BitmapSurface::UVShiftWidth()const{require_created();return half_texel_?.5f/float(image_.width):0;}
float BitmapSurface::UVShiftHeight()const{require_created();return half_texel_?.5f/float(image_.height):0;}
const RgbaImage &BitmapSurface::pixels()const{require_created();return image_;}
}
