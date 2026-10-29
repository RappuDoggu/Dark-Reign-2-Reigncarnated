#include "vid_slice.hpp"
#include "reign/bitmap_manager.hpp"
#include "bmp_fixture.hpp"
#include <array>
#include <iostream>
namespace {
std::vector<std::uint8_t> solid(bool green){std::vector<std::uint8_t> b(18);b[2]=2;b[12]=b[14]=1;b[16]=24;b[17]=32;b.insert(b.end(),{0,std::uint8_t(green?255:0),std::uint8_t(green?0:255)});return b;}
struct Replay {
 reign::LegacyGles *renderer=nullptr;bool bound=false;unsigned contexts=0;
 std::unique_ptr<reign::BitmapManager> manager;std::shared_ptr<const reign::ManagedBitmap> retained;
};
}
void original_bitmap_manager_gpu_checks(reign::LegacyGles &renderer){
 static Replay replay;replay.renderer=&renderer;replay.bound=true;++replay.contexts;
 reign::VidDevice device;reign::VidBinding binding(device);
 device.set_draw_sink([&](const auto *v,std::size_t n,const auto *ix,std::size_t ni,const auto &s){renderer.draw(32,32,v,n,ix,ni,s);});
 auto check=[](bool ok,const char *name){if(!ok)throw std::runtime_error(name);std::cout<<name<<": PASS\n";};
 try{
  if(!replay.manager)replay.manager=std::make_unique<reign::BitmapManager>([](const std::string &n){if(n=="bmp.bmp"){auto b=bmp_fixture();b[56]=255;return b;}return solid(n=="anim1.tga");},[](const std::string &n){return n=="anim.tga"||n=="anim1.tga"||n=="bmp.bmp";},reign::BitmapGpuCallbacks{
   [](const auto &image){return replay.renderer->texture_rgba(image.width,image.height,image.pixels.data());},
   [](unsigned id){replay.renderer->delete_texture(id);},
   []{if(replay.bound)Vid::SetTextureDX(nullptr,0,RS_BLEND_DEF);}});
  else replay.manager->restore();
  auto first=replay.manager->load("anim.xyz");auto frames=replay.manager->animation("anim");
  auto pixel=[&](const std::shared_ptr<const reign::ManagedBitmap> &bmp,bool green){
   if(!bmp||!bmp->resident||!bmp->texture)throw std::runtime_error("Bitmap has no live native texture");
   reign::TextureView texture(bmp->texture);Vid::SetTextureDX(&texture,0,RS_BLEND_DEF);Vid::SetAlphaState(FALSE);
   renderer.clear(32,32,0,0,0);reign::LegacyTL v[]={{-.5f,-.5f,.5f,1,0xffffffff,0xff000000,0,0},{31.5f,-.5f,.5f,1,0xffffffff,0xff000000,1,0},{-.5f,31.5f,.5f,1,0xffffffff,0xff000000,0,1}};std::uint16_t ix[]={0,1,2};
   Vid::DrawIndexedPrimitive(PT_TRIANGLELIST,FVF_TLVERTEX,v,3,ix,3,RS_BLEND_DEF);
   std::array<std::uint8_t,32*32*4> out;renderer.read_rgba(32,32,out.data());const auto at=((31-8)*32+8)*4;return out[at]==(green?0:255)&&out[at+1]==(green?255:0)&&out[at+2]==0;
  };
  check(first&&replay.manager->load("anim.pic")==first&&pixel(first,false),"Original bitmap manager fallback/cache to GLES");
  check(frames.size()==2&&frames[1]->frame==1&&pixel(frames[1],true),"Original bitmap manager numbered animation to GLES");
  auto bmp=replay.manager->load("bmp.xyz");check(bmp&&bmp->source=="bmp.bmp"&&pixel(bmp,false),"Original bitmap manager BMP fallback to GLES");
  auto generation=first->generation;replay.manager->release();if(first->resident||first->texture)throw std::runtime_error("Bitmap release retained live handles");replay.manager->restore();
  check(first->generation==generation+1&&pixel(first,false),"Original bitmap manager release/restore to GLES");
  const bool same=!replay.retained||replay.retained==first;replay.retained=first;replay.manager->abandon_context();
  check(same&&!first->texture&&!first->resident&&first->generation==replay.contexts*2,"Original bitmap manager context invalidation/replay");
  // These textures belonged to the context that gles_smoke now destroys. CPU records survive.
  replay.bound=false;replay.renderer=nullptr;
 }catch(...){if(replay.manager)replay.manager->clear();replay.bound=false;replay.renderer=nullptr;throw;}
}
