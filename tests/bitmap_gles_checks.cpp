#include "bucket.h"
#include "reign/bitmap_pixels.hpp"
#include <array>
#include <cmath>
#include <iostream>
void original_bitmap_gpu_checks(reign::LegacyGles &renderer){
 constexpr unsigned W=32,H=32;reign::VidDevice device;reign::VidBinding binding(device);
 device.set_draw_sink([&](const VertexTL *v,std::size_t n,const U16 *ix,std::size_t ni,const reign::DrawState &s){renderer.draw(W,H,v,n,ix,ni,s);});
 std::vector<std::array<U8,4>> palette(256);palette[1]={255,0,0,0};
 const std::vector<U8> sources[]={{1},{255,255},{0,255,0},{255,0,0,128}};
 const unsigned depths[]={8,16,24,32};const int expected[][3]={{255,0,0},{124,124,251},{0,255,0},{0,0,255}};
 for(unsigned test=0;test<4;++test){
  auto rgba=reign::convert_bitmap_pixels(1,1,depths[test],sources[test],reign::rgba8_format,palette);
  auto id=renderer.texture_rgba(1,1,rgba.data());reign::TextureView texture(id);
  {
   BucketMan manager;manager.Init(2,1000,.95f,TRUE);manager.Flush(FALSE);BucketMan::SetPrimitiveDesc(PT_TRIANGLELIST,FVF_TLVERTEX,RS_BLEND_DEF);BucketMan::SetTexture(&texture);
   Vid::SetAlphaState(TRUE);renderer.clear(W,H,0,0,1);
   void *p=nullptr;U16 *ix=nullptr;auto bucket=manager.LockIndexedPrimitiveMem(&p,3,&ix,3);if(!bucket)throw std::runtime_error("Bitmap GPU bucket allocation failed");
   auto v=static_cast<VertexTL *>(p);v[0]={-.5f,-.5f,.5f,1,0xffffffff,0xff000000,0,0};v[1]={31.5f,-.5f,.5f,1,0xffffffff,0xff000000,1,0};v[2]={-.5f,31.5f,.5f,1,0xffffffff,0xff000000,0,1};ix[0]=0;ix[1]=1;ix[2]=2;manager.UnlockIndexedPrimitiveMem(*bucket,3,3);manager.Flush();
   std::array<U8,W*H*4> pixels;renderer.read_rgba(W,H,pixels.data());auto at=((H-1-8)*W+8)*4;
   for(unsigned channel=0;channel<3;++channel)if(std::abs(int(pixels[at+channel])-expected[test][channel])>2)throw std::runtime_error("Original pixel conversion/bucket GPU mismatch");
  }
  renderer.delete_texture(id);std::cout<<"Original "<<depths[test]<<"-bit CopyLine -> bucket -> GLES: PASS\n";
 }
}
