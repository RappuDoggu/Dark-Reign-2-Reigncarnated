#include "bucket.h"
#include "tranbucket.h"
#include <array>
#include <cmath>
#include <iostream>
void original_bucket_gpu_checks(reign::LegacyGles &renderer){
 constexpr unsigned W=32,H=32;
 reign::VidDevice device;reign::VidBinding binding(device);unsigned calls=0;
 device.set_draw_sink([&](const VertexTL *v,std::size_t n,const U16 *ix,std::size_t ni,const reign::DrawState &state){renderer.draw(W,H,v,n,ix,ni,state);++calls;});
 auto append=[](BucketMan &manager,U32 color,F32 z){void *p=nullptr;U16 *ix=nullptr;auto b=manager.LockIndexedPrimitiveMem(&p,3,&ix,3);if(!b)throw std::runtime_error("No original GPU bucket memory");
  auto v=static_cast<VertexTL *>(p);v[0]={-.5f,-.5f,z,1,color,0xff000000,0,0};v[1]={31.5f,-.5f,z,1,color,0xff000000,1,0};v[2]={-.5f,31.5f,z,1,color,0xff000000,0,1};ix[0]=0;ix[1]=1;ix[2]=2;manager.UnlockIndexedPrimitiveMem(*b,3,3);};
 auto expect=[&](int r,int g,int b,const char *name){std::array<U8,W*H*4> pixels;renderer.read_rgba(W,H,pixels.data());auto p=((H-1-8)*W+8)*4;
  if(std::abs(int(pixels[p])-r)>2||std::abs(int(pixels[p+1])-g)>2||std::abs(int(pixels[p+2])-b)>2)throw std::runtime_error(std::string(name)+" pixel mismatch: "+std::to_string(pixels[p])+","+std::to_string(pixels[p+1])+","+std::to_string(pixels[p+2]));std::cout<<name<<": PASS\n";};
 {
  BucketMan manager;manager.Init(3,1000,.95f,TRUE);manager.Flush(FALSE);BucketMan::SetPrimitiveDesc(PT_TRIANGLELIST,FVF_TLVERTEX,RS_BLEND_DEF);
  renderer.clear(W,H,0,0,0);append(manager,0xffff0000,.5f);append(manager,0xff00ff00,.5f);manager.Flush();
  if(calls!=1)throw std::runtime_error("GPU compatible buckets not merged");expect(0,255,0,"Original bucket merge/rebase to GLES");
 }
 {
  TranBucketMan manager;manager.Init(3,1000,.95f,FALSE);manager.Flush(FALSE);BucketMan::SetPrimitiveDesc(PT_TRIANGLELIST,FVF_TLVERTEX,RS_BLEND_DEF);
  renderer.clear(W,H,0,0,0,0);append(manager,0x80ff0000,.2f);append(manager,0x800000ff,.8f);manager.Flush();expect(128,0,64,"Original translucent triangle sort/blend");
  renderer.clear(W,H,0,0,0,0);BucketMan::SetTag1(1);append(manager,0x80ff0000,.2f);BucketMan::SetTag1(9);append(manager,0x800000ff,.8f);manager.Flush();expect(128,0,64,"Original translucent bucket tag sort/blend");
  // Additive passes suppress colored fog, then restore the configured fog color.
  renderer.clear(W,H,0,0,0,0);BucketMan::SetFlags(RS_BLEND_ADD);device.SetRenderState(28,1);Vid::renderState.fogColor=0xff00ff00;Vid::SetFogColorD3D(Vid::renderState.fogColor);
  void *p=nullptr;U16 *ix=nullptr;auto bucket=manager.LockIndexedPrimitiveMem(&p,3,&ix,3);if(!bucket)throw std::runtime_error("No fog bucket");
  auto v=static_cast<VertexTL *>(p);v[0]={-.5f,-.5f,.5f,1,0xffffffff,0,0,0};v[1]={31.5f,-.5f,.5f,1,0xffffffff,0,0,0};v[2]={-.5f,31.5f,.5f,1,0xffffffff,0,0,0};ix[0]=0;ix[1]=1;ix[2]=2;manager.UnlockIndexedPrimitiveMem(*bucket,3,3);manager.Flush();expect(0,0,0,"Original additive fog suppression");
  U32 color=0;device.GetRenderState(34,&color);if(color!=0xff00ff00)throw std::runtime_error("Fog color not restored");std::cout<<"Original additive fog color restoration: PASS\n";
 }
}
