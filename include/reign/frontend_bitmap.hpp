#pragma once
#include "reign/bitmap_surface.hpp"
#include <memory>
#include <stdexcept>
// Static frontend texture adapter; does not pretend to decode Bink movies.
class Bitmap {
 std::shared_ptr<reign::BitmapSurface> surface_;
public:
 enum { reduceNONE=0 };
 struct Manager { static Bitmap *FindCreate(unsigned,const char *); };
 explicit Bitmap(std::shared_ptr<reign::BitmapSurface> surface):surface_(std::move(surface)){
  if(!surface_||surface_->Width()<=0||surface_->Height()<=0)throw std::invalid_argument("Created bitmap surface required");
 }
 float InvWidth()const{return 1.f/surface_->Width();}
 int Width()const{return surface_->Width();}
 int Height()const{return surface_->Height();}
 float InvHeight()const{return 1.f/surface_->Height();}
 float UVShiftWidth()const{return surface_->UVShiftWidth();}
 float UVShiftHeight()const{return surface_->UVShiftHeight();}
 const reign::BitmapSurface &surface()const{return *surface_;}
};
namespace reign {
class BitmapManager;
// Immutable CPU snapshots for static frontend images. Keep this object alive
// until all controls and their TextureInfo references have been destroyed.
class FrontendTextures {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 explicit FrontendTextures(BitmapManager &);~FrontendTextures();
 FrontendTextures(const FrontendTextures&)=delete;
 FrontendTextures &operator=(const FrontendTextures&)=delete;
 void run(const std::function<void()> &);
};
}
