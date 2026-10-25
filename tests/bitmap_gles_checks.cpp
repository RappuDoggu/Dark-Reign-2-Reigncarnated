#include "bucket.h"
#include "reign/bitmap_pixels.hpp"
#include "reign/bitmap_loader.hpp"
#include <array>
#include <cmath>
#include <iostream>
void original_bitmap_gpu_checks(reign::LegacyGles &renderer){
 constexpr unsigned W=32,H=32;reign::VidDevice device;reign::VidBinding binding(device);
 device.set_draw_sink([&](const VertexTL *v,std::size_t n,const U16 *ix,std::size_t ni,const reign::DrawState &s){renderer.draw(W,H,v,n,ix,ni,s);});
 std::vector<std::array<U8,4>> palette(256);palette[1]={255,0,0,0};
 const std::vector<U8> sources[]={{1},{255,255},{0,255,0},{255,0,0,128},{0,255,255,128}};
 const unsigned depths[]={8,16,24,32,32};const int expected[][3]={{255,0,0},{124,124,251},{0,255,0},{0,0,255},{128,128,127}};
 for(unsigned test=0;test<5;++test){
  std::vector<U8> tga(18);tga[2]=test==0?1:2;tga[12]=tga[14]=1;tga[16]=depths[test];tga[17]=32;
  if(test==0){tga[1]=1;tga[3]=1;tga[5]=1;tga[7]=32;tga.insert(tga.end(),{0,0,255,0});}
  tga.insert(tga.end(),sources[test].begin(),sources[test].end());
  auto rgba=reign::load_bitmap_tga(tga).pixels;
  if(test==4){std::vector<U8> pic(104);pic[0]=0x53;pic[1]=0x80;pic[2]=0xf6;pic[3]=0x34;pic[88]='P';pic[89]='I';pic[90]='C';pic[91]='T';pic[93]=pic[95]=1;
   pic.insert(pic.end(),{1,8,0,0xe0,0,8,0,0x10,255,255,0,128});rgba=reign::load_bitmap_pic(pic).pixels;}
  if(rgba!=reign::convert_bitmap_pixels(1,1,depths[test],sources[test],reign::rgba8_format,palette))throw std::runtime_error("TGA row loading mismatch");
  auto id=renderer.texture_rgba(1,1,rgba.data());reign::TextureView texture(id);
  {
   BucketMan manager;manager.Init(2,1000,.95f,TRUE);manager.Flush(FALSE);BucketMan::SetPrimitiveDesc(PT_TRIANGLELIST,FVF_TLVERTEX,RS_BLEND_DEF);BucketMan::SetTexture(&texture);
   Vid::SetAlphaState(TRUE);renderer.clear(W,H,0,0,1);
   void *p=nullptr;U16 *ix=nullptr;auto bucket=manager.LockIndexedPrimitiveMem(&p,3,&ix,3);if(!bucket)throw std::runtime_error("Bitmap GPU bucket allocation failed");
   auto v=static_cast<VertexTL *>(p);v[0]={-.5f,-.5f,.5f,1,0xffffffff,0xff000000,0,0};v[1]={31.5f,-.5f,.5f,1,0xffffffff,0xff000000,1,0};v[2]={-.5f,31.5f,.5f,1,0xffffffff,0xff000000,0,1};ix[0]=0;ix[1]=1;ix[2]=2;manager.UnlockIndexedPrimitiveMem(*bucket,3,3);manager.Flush();
   std::array<U8,W*H*4> pixels;renderer.read_rgba(W,H,pixels.data());auto at=((H-1-8)*W+8)*4;
   for(unsigned channel=0;channel<3;++channel)if(std::abs(int(pixels[at+channel])-expected[test][channel])>2)throw std::runtime_error("Original pixel conversion/bucket GPU mismatch");
  }
  renderer.delete_texture(id);std::cout<<(test==4?"PIC alpha":std::to_string(depths[test])+"-bit TGA")<<" -> original CopyLine -> bucket -> GLES: PASS\n";
 }
}
