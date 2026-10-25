#include "vid_slice.hpp"
#include "reign/native_font.hpp"
#include "reign/font_registry.hpp"
#include "font_fixture.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
namespace {
void render_font(reign::LegacyGles &renderer,const reign::NativeFont &font,const std::u16string &text,reign::FontDrawOptions options,unsigned expected_batches=1){
 std::vector<unsigned> textures;font.upload([&](unsigned index,const reign::RgbaImage &image){if(index!=textures.size())throw std::runtime_error("Font atlas upload order");textures.push_back(renderer.texture_rgba(image.width,image.height,image.pixels.data()));});
 reign::VidDevice device;reign::VidBinding binding(device);Vid::SetAlphaState(TRUE);
 device.set_draw_sink([&](const reign::LegacyTL *v,std::size_t n,const std::uint16_t *i,std::size_t ni,const reign::DrawState &state){renderer.draw(32,32,v,n,i,ni,state);});
 unsigned batches=0;
 try{font.draw(text,options,[&](const reign::FontBatch &batch){
  if(batch.filtered||batch.atlas>=textures.size())throw std::runtime_error("Unexpected font sampler/atlas");
  reign::TextureView texture(textures[batch.atlas]);Vid::SetTextureDX(&texture,0,RS_BLEND_DEF);
  // Original font/IFace batches enter the already tested original Vid submission.
  Vid::DrawIndexedPrimitive(PT_TRIANGLELIST,FVF_TLVERTEX,const_cast<reign::LegacyTL *>(batch.vertices.data()),batch.vertices.size(),const_cast<std::uint16_t *>(batch.indices.data()),batch.indices.size(),batch.flags);++batches;
 });}catch(...){for(auto id:textures)renderer.delete_texture(id);throw;}
 for(auto id:textures)renderer.delete_texture(id);
 if(batches!=expected_batches)throw std::runtime_error("Original font GPU batch count");
}
}
void original_font_gpu_checks(reign::LegacyGles &renderer){
 // The same CPU font survives both EGL contexts in gles_smoke; GPU handles do not.
 static const reign::NativeFont font(font_fixture(2,2));
 std::array<std::uint8_t,32*32*4> pixels;
 auto pixel=[&](unsigned x,unsigned y,int r,int g,int b){renderer.read_rgba(32,32,pixels.data());auto at=((31-y)*32+x)*4;bool ok=std::abs(int(pixels[at])-r)<=2&&std::abs(int(pixels[at+1])-g)<=2&&std::abs(int(pixels[at+2])-b)<=2;if(!ok){std::cerr<<"font pixel "<<x<<","<<y<<" got "<<int(pixels[at])<<","<<int(pixels[at+1])<<","<<int(pixels[at+2])<<" expected "<<r<<","<<g<<","<<b<<"\n";for(unsigned yy=3;yy<8;++yy){for(unsigned xx=3;xx<8;++xx)std::cerr<<int(pixels[((31-yy)*32+xx)*4])<<" ";std::cerr<<"\n";}}return ok;};
 auto check=[](bool ok,const char *name){if(!ok)throw std::runtime_error(name);std::cout<<name<<": PASS\n";};
 reign::FontDrawOptions options;options.x=4;options.y=4;options.clip={{0,0},{32,32}};
 renderer.clear(32,32,0,0,0);render_font(renderer,font,u"A",options);
 check(pixel(4,4,255,255,255)&&pixel(5,4,128,128,128)&&pixel(4,5,64,64,64)&&pixel(5,5,0,0,0),"Original font atlas alpha/half-texel pixels and context replay");
 renderer.clear(32,32,0,0,0);options.argb=0xffff0000;options.alpha_scale=.5f;render_font(renderer,font,u"A",options);
 check(pixel(4,4,127,0,0),"Original font tint and alpha scale");
 auto opaque=font_fixture(2,1);std::fill(opaque.end()-4,opaque.end(),255);reign::NativeFont shadow(opaque);
 options.argb=0xffffffff;options.alpha_scale=1;options.shadow=1;options.shadow_alpha=77;renderer.clear(32,32,0,0,0);render_font(renderer,shadow,u"A",options);
 check(pixel(4,4,255,255,255)&&pixel(5,5,178,178,178),"Original font shadow order/opacity");
 options.shadow=0;options.clip.p1.x=6;renderer.clear(32,32,0,0,0);render_font(renderer,font,u"AB",options);
 check(pixel(4,4,255,255,255)&&pixel(8,4,0,0,0),"Original font right-clip early return");
 auto paged=font_fixture(2,700,4);std::fill(paged.end()-16,paged.end(),128);reign::NativeFont pages(paged);
 options.clip.p1.x=32;options.x=2;options.y=2;renderer.clear(32,32,0,0,0);render_font(renderer,pages,u"A\u02fc",options,2);
 check(pages.atlas_count()==2&&pixel(2,2,255,255,255)&&pixel(7,2,128,128,128),"Original font multi-atlas texture switch");
 reign::FontRegistry registry([](const std::string &name){if(name=="fonts.cfg"){std::string config="ConfigureInterface() { CreateFont(\"System\") { File(\"one.fff\"); } CreateFont(\"Button\") { File(\"one.fff\"); } }";return std::vector<std::uint8_t>(config.begin(),config.end());}return font_fixture();});
 if(registry.configure("fonts.cfg")!=2)throw std::runtime_error("Font configuration dispatch failed");
 auto registered=registry.find("Button");registry.reload();registered=registry.find("System");
 options.x=4;options.y=4;renderer.clear(32,32,0,0,0);render_font(renderer,*registered,u"A",options);
 check(registry.file_count()==1&&registered==registry.find("Button")&&pixel(4,4,255,255,255)&&pixel(5,4,128,128,128),"Original font configuration/alias/reload to GLES");

}
