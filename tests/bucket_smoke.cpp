#include "bucket.h"
#include "tranbucket.h"
#include <iostream>
#include <vector>
namespace {
void check(bool b,const char *s){if(!b)throw std::runtime_error(s);}
template<class F>void rejects(F f){try{f();}catch(const std::runtime_error&){return;}throw std::runtime_error("Unsupported bucket input accepted");}
struct Draw {std::vector<VertexTL> vertices;std::vector<U16> indices;reign::DrawState state;};
Bucket *append(BucketMan &manager,F32 z=.5f,U32 color=0xffffffff,Bool rebase=TRUE){
 void *memory=nullptr;U16 *indices=nullptr;auto bucket=manager.LockIndexedPrimitiveMem(&memory,3,&indices,3);
 check(bucket,"No bucket memory");auto v=static_cast<VertexTL *>(memory);
 for(U32 i=0;i<3;++i){v[i]={float(i),float(i),z,1,color,0xff000000,0,0};indices[i]=static_cast<U16>(i+(rebase?0:bucket->vCount));}
 manager.UnlockIndexedPrimitiveMem(*bucket,3,3,rebase);return bucket;
}
}
int main(){try{
 reign::VidDevice device;reign::VidBinding binding(device);std::vector<Draw> draws;
 device.set_draw_sink([&](const VertexTL *v,std::size_t n,const U16 *ix,std::size_t ni,const reign::DrawState &state){draws.push_back({{v,v+n},{ix,ix+ni},state});});
 // Exercise the allocator's split rounding across many small pool sizes.
 for(U32 size=512;size<=1056;size+=4){
  BucketMan manager;manager.Init(3,size,.95f,TRUE);manager.Flush(FALSE);
  BucketMan::SetPrimitiveDesc(PT_TRIANGLELIST,FVF_TLVERTEX,RS_BLEND_DEF);
  auto b=append(manager);check(reinterpret_cast<std::uintptr_t>(b->iMem)%4==0,"Index split alignment");append(manager,.8f,0xffffffff,FALSE);
  manager.Flush();check(draws.size()==1&&draws[0].vertices.size()==6&&draws[0].indices==std::vector<U16>({0,1,2,3,4,5}),"Merged batch and rebased indices");draws.clear();
 }
 {
  BucketMan manager;manager.Init(3,1000,.95f,TRUE);manager.Flush(FALSE);
  BucketMan::SetPrimitiveDesc(PT_TRIANGLELIST,FVF_TLVERTEX,RS_BLEND_DEF);append(manager);
  BucketMan::SetFlags(RS_BLEND_GLOW);append(manager);manager.Flush();check(draws.size()==2,"Different state must separate buckets");draws.clear();
  append(manager);manager.Flush(FALSE);check(draws.empty(),"Discard flush submitted geometry");
  BucketMan::SetFlags(RS_BLEND_DEF);auto b=append(manager,.2f);append(manager,.8f);b->Sort();
  check(b->iMem[0]==3&&b->iMem[3]==0,"Original triangle depth sort");manager.Flush();draws.clear();
  void *v=nullptr;U16 *ix=nullptr;b=manager.LockIndexedPrimitiveMem(&v,3,&ix,3);ix[0]=99;ix[1]=1;ix[2]=2;
  rejects([&]{manager.UnlockIndexedPrimitiveMem(*b,3,3);});manager.UnlockIndexedPrimitiveMem(*b,0,0);
  rejects([&]{manager.LockIndexedPrimitiveMem(&v,0xffffffffu,&ix,3);});
  BucketMan::SetFlags(RS_BLEND_DEF|RS_NOINDEXED);b=manager.LockPrimitiveMem(&v,3);check(b,"Nonindexed lock");
  for(unsigned i=0;i<3;++i)static_cast<VertexTL *>(v)[i]={float(i),float(i),.5f,1,0xffffffff,0xff000000,0,0};
  manager.UnlockPrimitiveMem(*b,3);manager.Flush();check(draws.size()==1&&draws[0].indices.size()==3,"Nonindexed TL submission");draws.clear();
 }
 {
  TranBucketMan manager;manager.Init(450,256,.95f,FALSE);manager.Flush(FALSE);
  BucketMan::SetPrimitiveDesc(PT_TRIANGLELIST,FVF_TLVERTEX,RS_BLEND_DEF);
  for(U32 i=0;i<450;++i){BucketMan::SetTag1(static_cast<U16>(i));append(manager,float(i)/1000,0xff000000u|i);}
  manager.Flush();check(draws.size()==450,"Dynamic translucent sorting list");
  for(U32 i=0;i<450;++i)check((draws[i].vertices[0].diffuse&0xffffu)==449-i,"Original translucent tag ordering");
  U32 alpha=9;device.GetRenderState(27,&alpha);check(alpha==0,"Translucent alpha restoration");draws.clear();
 }
 {
  BucketMan manager;manager.Init(3,1000,.95f,TRUE);manager.Flush(FALSE);
  Bitmap first(1),second(2);BucketMan::SetPrimitiveDesc(PT_TRIANGLELIST,FVF_TLVERTEX,RS_BLEND_DEF);
  BucketMan::SetTexture(&first,0);BucketMan::SetTexture(&second,1);append(manager);
  rejects([&]{manager.Flush();});manager.Flush(FALSE);check(draws.empty(),"Unsupported multi-stage discard submitted geometry");
 }
 rejects([&]{BucketMan manager;manager.Init(3,1001,.95f,TRUE);});
 rejects([&]{BucketMan manager;manager.Init(0xffffffffu,1000,.95f,TRUE);});
 std::cout<<"Original bucket split alignment/merge/rebase/state/sort/450-bucket flush and rejection checks PASS\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
