#pragma once
#include "reign/tokenizer_prelude.hpp"
#include "reign/engine_file.hpp"
using U16=std::uint16_t;
using S32=std::int32_t;
constexpr U32 U16_MAX=65535;
#define ERR_FATAL(args) reign::config_error args;
#define DEVELOPMENT 1
#include "reign/native_font.hpp"
#include <array>
#include <cmath>
namespace reign::font_detail {
using PathString=StrBuf<270>;
using ClipRect=reign::PixelRect;
using VertexTL=reign::LegacyTL;
struct Color {
 U8 b=0,g=0,r=0,a=0;
 Color()=default;
 Color(U32 color):b(U8(color)),g(U8(color>>8)),r(U8(color>>16)),a(U8(color>>24)){}
 operator U32()const{return (U32(a)<<24)|(U32(r)<<16)|(U32(g)<<8)|b;}
 void Set(U32 red,U32 green,U32 blue,U32 alpha){r=U8(red);g=U8(green);b=U8(blue);a=U8(alpha);}
};
inline U16 load16(const U8 *p){U16 value;std::memcpy(&value,p,2);return value;}
inline U32 load32(const U8 *p){U32 value;std::memcpy(&value,p,4);return value;}
namespace Utils {inline S32 FtoL(F32 value){return static_cast<S32>(value);}}
constexpr U32 bitmapTEXTURE=2,bitmapNOMANAGE=16;
class Bitmap;
struct Context {
 std::array<Bitmap *,1000> textures{};U16 currTex=0;
 std::vector<std::unique_ptr<Bitmap>> owned;
 std::array<bool,1000> published{};bool half_texel=true,drawing=false;
 std::vector<VertexTL> vertices;std::vector<U16> indices;
 Bitmap *current_texture=nullptr;U32 filter=0;
 FontDrawOptions options;std::function<void(const FontBatch &)> sink;
 ~Context();
};
extern thread_local Context *active;
Context &context();
struct Binding {Context *previous;explicit Binding(Context &);~Binding();};
class Bitmap:public reign::BitmapSurface {
public:
 unsigned handle;
 explicit Bitmap(U32 flags);
};
namespace FontSys {Bitmap *AllocTexture(U16 &);Bitmap *FindTexture(U16);}
namespace Vid {
struct State {U32 maxVerts=4096,maxIndices=8193;};
extern thread_local State renderState;
constexpr U32 PT_TRIANGLELIST=4,FVF_TLVERTEX=0x1c4,DP_DONOTUPDATEEXTENTS=8,RS_2SIDED=0x200,RS_BLEND_DEF=0x30650000,filterFILTER=1;
extern U16 rectIndices[12];
U32 SetFilterStateI(U32);void SetTexture(Bitmap *);void SetMaterial(void *);
void DrawIndexedPrimitive(U32,U32,VertexTL *,U32,U16 *,U32,U32);
struct BackBitmap {const ClipRect &GetClipRect()const;};extern BackBitmap backBmp;
namespace Heap {U32 ReqVertex(VertexTL **,U16 **);void Restore(U32);}
}
// These flag names are referenced unqualified in the original IFace slice.
using namespace Vid;
namespace IFace {
constexpr U32 SHADOW_ALPHA=0;
S32 GetMetric(U32);
VertexTL *GetVerts(U32,Bitmap *,U32,U32,U16 &);
void SetIndex(const U16 *,U32,U16);
void InitBuckets();void DoneBuckets();void AbortBuckets()noexcept;
}
}
