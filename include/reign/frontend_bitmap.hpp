#pragma once
#include "reign/bitmap_surface.hpp"
#include <memory>
#include <stdexcept>
// Static frontend texture adapter; does not pretend to decode Bink movies.
class Bitmap {
 std::shared_ptr<reign::BitmapSurface> surface_;
public:
 explicit Bitmap(std::shared_ptr<reign::BitmapSurface> surface):surface_(std::move(surface)){
  if(!surface_||surface_->Width()<=0||surface_->Height()<=0)throw std::invalid_argument("Created bitmap surface required");
 }
 float InvWidth()const{return 1.f/surface_->Width();}
 float InvHeight()const{return 1.f/surface_->Height();}
 float UVShiftWidth()const{return surface_->UVShiftWidth();}
 float UVShiftHeight()const{return surface_->UVShiftHeight();}
 const reign::BitmapSurface &surface()const{return *surface_;}
};
