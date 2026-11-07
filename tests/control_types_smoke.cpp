#include "control_types.hpp"
#include <iostream>
#include <type_traits>

static_assert(sizeof(Color)==4, "Original packed ARGB color must remain 32 bits");
static_assert(sizeof(Point<S32>)==8 && sizeof(Area<S32>)==16);
static_assert(alignof(PaintInfo)>=alignof(void*), "Native control pointers must remain aligned");
static_assert(std::is_same_v<decltype(std::declval<IControl>().dTrack.trackPos),U32*>);
static_assert(std::is_same_v<decltype(std::declval<const IControl>().GetScreenPos()),Point<S32>>);

int main(){try{
 unsigned checks=0;
 auto check=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Control type check "+std::to_string(checks));};
 // Original controls use inclusive bounds. Keep corner behavior distinct from
 // half-open image storage and framebuffer bounds.
 Area<S32> rect(10,20,50,80);
 check(rect.Width()==40 && rect.Height()==60 && rect.MidX()==30 && rect.MidY()==50);
 check(rect.In(Point<S32>(10,20)) && rect.In(Point<S32>(50,80)));
 check(!rect.In(Point<S32>(51,80)) && !rect.In(Point<S32>(10,19)));
 auto shifted=rect+Point<S32>(-20,5);
 check(shifted.p0.x==-10 && shifted.p0.y==25 && shifted.p1.x==30 && shifted.p1.y==85);
 Area<S32> clipped(-5,15,30,100);
 check(clipped.Clip(rect) && clipped.p0==Point<S32>(10,20) && clipped.p1==Point<S32>(30,80));
 Area<S32> outside(70,90,100,120);
 check(!outside.Clip(rect) && outside.Width()==0 && outside.Height()==0);
 Area<S32> reversed(50,80,10,20);reversed.Sort();check(reversed.p0==rect.p0 && reversed.p1==rect.p1);
 Area<F32> uv(.25f,.125f,.75f,.875f);check(uv.Width()==.5f && uv.Height()==.75f);
 Color packed(0x7f123456u);check(packed.a==0x7f && packed.r==0x12 && packed.g==0x34 && packed.b==0x56);
 Color rgba(U32(18),U32(52),U32(86),U32(127));check(rgba.color==packed.color);
 // Verify the original double-exponent color conversion, not a replacement
 // that silently truncates 0.5 * 255 to 127.
 std::fesetround(FE_TONEAREST);
 Color fractional(.5f,.25f,1.f,U32(255));
 check(fractional.r==128 && fractional.g==64 && fractional.b==255 && fractional.a==255);
 TextureInfo texture;check(!texture.texture && texture.texMode==TextureInfo::TM_STRETCHED && !texture.filter && !texture.system);
 ColorGroup group;PaintInfo info(&group);check(!info.font && info.colors==&group && info.alphaScale==1.f);
 TextureSkin skin;check(skin.border.p0==Point<S32>(0,0) && skin.border.p1==Point<S32>(0,0));
 check(IControl::ClassId()==0x05313784 && std::string(IControl::ClassName())=="IControl");
 std::cout<<"Original control declarations/geometry bounds/packed and fractional colors: PASS ("<<checks<<" checks)\n";
 std::cout<<"Full IControl construction/drawing/event loop/menu: not linked or tested\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
