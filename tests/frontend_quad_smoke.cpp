#include "reign/frontend_draw.hpp"
#include "reign/frontend_bitmap.hpp"
#include <iostream>
int main(){try{
 unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::logic_error("Quad check "+std::to_string(checks));};
 auto rejects=[&](auto f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}check(caught);};
 ClipRect rect;rect.p0={10,20};rect.p1={42,36};std::vector<reign::FrontendBatch> batches;
 auto sink=[&](const auto &b){batches.push_back(b);};
 reign::FrontendDraw::run_batches([&]{IFace::RenderRectangle(rect,Color(U32(0x80402010)),nullptr,.5f);},sink);
 check(batches.size()==1);auto &quad=batches[0];check(quad.vertices.size()==4&&quad.indices==std::vector<std::uint16_t>({0,1,2,0,2,3}));
 check(quad.vertices[0].x==10&&quad.vertices[0].y==20&&quad.vertices[2].x==42&&quad.vertices[2].y==36);
 for(auto &v:quad.vertices)check(v.diffuse==0x40402010&&v.z==0&&v.rhw==1&&v.specular==0xff000000);
 batches.clear();reign::FrontendDraw::run_batches([&]{IFace::RenderGradient(rect,Color(U32(0xffff0000)),Color(U32(0xff0000ff)),TRUE,nullptr,1);},sink);
 check(batches[0].vertices[1].diffuse==0xffff0000&&batches[0].vertices[3].diffuse==0xff0000ff);
 batches.clear();reign::FrontendDraw::run_batches([&]{IFace::RenderGradient(rect,Color(U32(0xffff0000)),Color(U32(0xff0000ff)),FALSE,nullptr,1);},sink);
 check(batches[0].vertices[1].diffuse==0xff0000ff&&batches[0].vertices[3].diffuse==0xffff0000);
 batches.clear();reign::FrontendDraw::run_batches([&]{IFace::RenderGradient(rect,Color(U32(0xff808080)),128,TRUE,nullptr,1);},sink);
 check(batches[0].vertices[0].diffuse==0xffc0c0c0&&batches[0].vertices[2].diffuse==0xff404040);
 Color corners[4]={Color(U32(0xff010101)),Color(U32(0xff020202)),Color(U32(0xff030303)),Color(U32(0xff040404))};
 batches.clear();reign::FrontendDraw::run_batches([&]{IFace::RenderGradient(rect,corners,nullptr);},sink);
 for(unsigned i=0;i<4;++i)check(batches[0].vertices[i].diffuse==U32(corners[i<2?i:5-i]));
 auto surface=std::make_shared<reign::BitmapSurface>([](const auto &){});surface->Create(8,4,true);Bitmap bitmap(surface);
 TextureInfo texture;texture.texture=&bitmap;texture.texMode=TextureInfo::TM_TILED;texture.filter=TRUE;
 batches.clear();reign::FrontendDraw::run_batches([&]{IFace::RenderRectangle(rect,Color(U32(0xffffffff)),&texture,1);},sink);
 check(batches[0].texture==&bitmap&&batches[0].filtered&&!batches[0].clamped);
 check(batches[0].vertices[2].u==4+bitmap.UVShiftWidth()&&batches[0].vertices[2].v==4+bitmap.UVShiftHeight());
 texture.texMode=TextureInfo::TM_CENTRED;texture.pixels.p0={0,0};texture.pixels.p1={8,4};texture.uv.p0={.1f,.2f};texture.uv.p1={.8f,.9f};
 batches.clear();reign::FrontendDraw::run_batches([&]{IFace::RenderRectangle(rect,Color(U32(0xffffffff)),&texture,1);},sink);
 check(batches[0].clamped&&batches[0].vertices[0].x==22&&batches[0].vertices[0].y==26);
 check(batches[0].vertices[2].u==.8f&&batches[0].vertices[2].v==.9f);
 ClipRect shadow=rect;shadow.p1.x+=2;shadow.p1.y+=2;batches.clear();
 reign::FrontendDraw::run_batches([&]{IFace::RenderShadow(rect,shadow,Color(U32(0x80000000)),2,1);},sink);check(batches.size()==2);
 check(batches[0].vertices[0].y==36&&batches[1].vertices[0].x==42);
 VertexTL clip[4]{};rejects([&]{reign::FrontendDraw::run_batches([&]{IFace::RenderRectangle(rect,Color(U32(0xffffffff)),nullptr,1,clip);},sink);});
 std::cout<<"Original UI quads/gradients/shadows/texture UV: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
