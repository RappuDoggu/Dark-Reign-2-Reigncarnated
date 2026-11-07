#include "reign/bitmap_surface.hpp"
#include <iostream>
#include <stdexcept>
namespace {template<class F>void rejects(F f){try{f();}catch(const std::runtime_error&){return;}throw std::runtime_error("Invalid bitmap operation accepted");}}
int main(){try{
 unsigned publications=0;reign::BitmapSurface surface([&](const reign::RgbaImage &p){if(p.pixels!=std::vector<std::uint8_t>({1,2,3,4,9,8,7,6}))throw std::runtime_error("Native bitmap content");++publications;});
 rejects([&]{surface.Lock();});surface.Create(2,1,true);surface.Clear(reign::BitmapSurface::MakeRGBA(1,2,3,4));surface.Lock();
 rejects([&]{surface.Lock();});rejects([&]{surface.Publish();});rejects([&]{surface.Create(1,1,true);});rejects([&]{surface.PutPixel(-1,0,0,nullptr);});
 auto rect=surface.GetClipRect();surface.PutPixel(1,0,reign::BitmapSurface::MakeRGBA(9,8,7,6),&rect);surface.UnLock();surface.Publish();surface.Publish();
 if(publications!=2||surface.UVShiftWidth()!=.25f)throw std::runtime_error("Bitmap publication/half texel");
 rejects([&]{surface.UnLock();});rejects([&]{surface.Create(16384,16384,true);});rejects([&]{reign::BitmapSurface::MakeRGBA(256,0,0);});
 reign::BitmapSurface unbound({});unbound.Create(1,1,true);rejects([&]{unbound.Publish();});
 std::cout<<"Native RGBA bitmap storage/lifecycle/bounds/publication replay PASS\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
