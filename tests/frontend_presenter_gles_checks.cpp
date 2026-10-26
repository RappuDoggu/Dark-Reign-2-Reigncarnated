#include "reign/frontend_presenter.hpp"
#include "reign/frontend_font.hpp"
#include "reign/frontend_draw.hpp"
#include "font_fixture.hpp"
#include <array>
#include <cmath>
#include <iostream>
void original_presenter_gpu_checks(reign::LegacyGles &renderer){
 reign::FrontendPresenter presenter(renderer);
 reign::FontRegistry registry([](const std::string &){return font_fixture(2,2);});registry.create("System","fixture.fff");
 reign::FrontendFonts fonts(registry,[&](const auto &font,const auto &batch){presenter.submit(font,batch);});fonts.set_frame({{0,0},{32,32}},128);
 std::array<std::uint8_t,32*32*4> pixels{};
 auto pixel=[&](unsigned x,unsigned y,int r,int g,int b){auto at=((31-y)*32+x)*4;return std::abs(int(pixels[at])-r)<=2&&std::abs(int(pixels[at+1])-g)<=2&&std::abs(int(pixels[at+2])-b)<=2;};
 for(unsigned frame=0;frame<2;++frame){
  presenter.begin(32,32);fonts.run([&]{presenter.draw([&]{
   IFace::RenderRectangle(ClipRect(0,0,32,32),Color(U32(0xffff0000)),nullptr,1);
   CH letter[]={'A',0};FontSys::GetFont("System")->Draw(4,4,letter,1,Color(U32(0xffffffff)));
   IFace::RenderRectangle(ClipRect(4,4,5,5),Color(U32(0xff00ff00)),nullptr,1);
  });});presenter.present();renderer.read_rgba(32,32,pixels.data());
  if(!pixel(4,4,0,255,0)||!pixel(5,4,255,128,128)||!pixel(4,5,255,64,64)||!pixel(8,8,255,0,0))throw std::runtime_error("Original UI background/text/foreground presentation order");
 }
 presenter.begin(32,32);bool failed=false;try{presenter.draw([]{throw std::runtime_error("Draw failure");});}catch(const std::runtime_error&){failed=true;}
 if(!failed)throw std::runtime_error("UI failure not propagated");presenter.begin(32,32);presenter.present();
 std::cout<<"Original UI ordered geometry/font GLES presenter and frame recovery: PASS\n";
}
