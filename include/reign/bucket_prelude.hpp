#pragma once
#include "vid_slice.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <limits>
#include <stdexcept>
#include <vector>
#undef ASSERT
#define ASSERT(condition) do {if(!(condition))throw std::runtime_error("Original bucket assertion: " #condition);}while(false);
using U8=std::uint8_t;
using U16=std::uint16_t;
using S32=std::int32_t;
using F32=float;
using Bitmap=reign::TextureView;
using VertexTL=reign::LegacyTL;
class BucketMan;
class Material;
constexpr U32 U16_MAX=65535;
constexpr U32 MAX_TEXTURE_STAGES=3,D3DPT_TRIANGLELIST=4;
#define _cdecl
namespace reign::bucket {
inline void log(const char *fmt,...){va_list args;va_start(args,fmt);std::vfprintf(stderr,fmt,args);va_end(args);std::fputc('\n',stderr);}
[[noreturn]] inline void fatal(const char *fmt,...){char message[1024];va_list args;va_start(args,fmt);std::vsnprintf(message,sizeof(message),fmt,args);va_end(args);throw std::runtime_error(message);}
inline void validate_init(U32 count,U32 size,F32 ratio){
 if(!count||!size||size%4||!std::isfinite(ratio)||ratio<=0||ratio>1||count>10000||std::uint64_t(count+5)*size>128*1024*1024)throw std::runtime_error("Unsupported bucket pool dimensions");
}
inline void validate_request(U32 vertices,U32 indices){
 if(vertices>65533||indices>0x7fffffffu||std::uint64_t(vertices+3)*sizeof(VertexTL)+std::uint64_t(indices+1)*2>128*1024*1024)throw std::runtime_error("Bucket request exceeds native batch limits");
}
}
#undef ERR_FATAL
#define ERR_FATAL(args) reign::bucket::fatal args
#define LOG_WARN(args) reign::bucket::log args
#define LOG_DIAG(args) reign::bucket::log args
namespace Utils {
inline U32 FtoLDown(F32 value){if(!std::isfinite(value)||value<0||double(value)>std::numeric_limits<U32>::max())throw std::runtime_error("Bucket size conversion out of range");return static_cast<U32>(std::floor(value));}
}
namespace Debug::Memory::Aligning {
inline void *AligningAlloc(U32 size,U32 alignment){void *p=nullptr;if(posix_memalign(&p,alignment<sizeof(void*)?sizeof(void*):alignment,size))throw std::bad_alloc();return p;}
inline void AligningFree(void *p){std::free(p);}
}
namespace Vid {
struct NativeCaps {Bool noTransort=FALSE;};
inline thread_local NativeCaps caps;
inline void SetFogColorD3D(U32 color){device->SetRenderState(34,color);}
inline Bool SetTexture(Bitmap *texture,U32 stage,U32 blend){return SetTextureDX(texture,stage,blend);}
// Real conversion for the supported nonindexed TL triangle list; other layouts reject.
inline Bool DrawPrimitive(PRIMITIVE_TYPE type,VERTEX_TYPE fvf,void *data,U32 count,U32 flags){
 if(type!=PT_TRIANGLELIST||fvf!=FVF_TLVERTEX||!count||count>65536||count%3)throw std::runtime_error("Unsupported nonindexed bucket primitive");
 std::vector<U16> indices(count);for(U32 i=0;i<count;++i)indices[i]=static_cast<U16>(i);
 return DrawIndexedPrimitive(type,fvf,data,count,indices.data(),count,flags);
}
}
