#pragma once
#include "reign/vid_device.hpp"
#include <stdexcept>
using U32=std::uint32_t;
using DWORD=std::uint32_t;
using LPVOID=void *;
using LPWORD=std::uint16_t *;
using PRIMITIVE_TYPE=U32;
using D3DPRIMITIVETYPE=U32;
using VERTEX_TYPE=U32;
constexpr U32 PT_POINTLIST=1,PT_LINELIST=2,PT_LINESTRIP=3,PT_TRIANGLELIST=4,PT_TRIANGLESTRIP=5,PT_TRIANGLEFAN=6;
constexpr U32 FVF_TLVERTEX=0x1c4;
constexpr U32 DP_DONOTCLIP=4,DP_MASK=1,D3DRENDERSTATE_CLIPPING=136;
constexpr int D3D_OK=0;
#define DODXLEANANDGRUMPY 1
#define ASSERT(condition) do {if(!(condition))throw std::runtime_error("Original Vid assertion: " #condition);}while(false)
using Bool=std::uint32_t;
constexpr Bool FALSE=0,TRUE=1;
// Numeric interoperability constants from the inspected DX7 state ABI.
constexpr U32 D3DRENDERSTATE_ZENABLE=7,D3DRENDERSTATE_ZWRITEENABLE=14,
 D3DRENDERSTATE_SRCBLEND=19,D3DRENDERSTATE_DESTBLEND=20,D3DRENDERSTATE_ALPHABLENDENABLE=27;
constexpr U32 D3DZB_FALSE=0,D3DZB_TRUE=1,D3DZB_USEW=2;
constexpr U32 D3DTSS_COLOROP=1,D3DTSS_COLORARG1=2,D3DTSS_COLORARG2=3,
 D3DTSS_ALPHAOP=4,D3DTSS_ALPHAARG1=5,D3DTSS_ALPHAARG2=6,D3DTSS_ADDRESS=12;
constexpr U32 D3DTA_DIFFUSE=0,D3DTA_CURRENT=1,D3DTA_TEXTURE=2;
using TextureHandle=unsigned;
constexpr int DD_OK=0;
constexpr U32 D3DTOP_DISABLE=1,D3DTOP_SELECTARG1=2,D3DTOP_SELECTARG2=3,D3DTOP_MODULATE=4,
 D3DTOP_MODULATE2X=5,D3DTOP_MODULATE4X=6,D3DTOP_ADD=7,D3DTOP_ADDSIGNED=8,D3DTOP_BLENDTEXTUREALPHA=13;
constexpr U32 D3DTADDRESS_WRAP=1,D3DTADDRESS_MIRROR=2,D3DTADDRESS_CLAMP=3;
enum : U32 {D3DBLEND_ZERO=1,D3DBLEND_ONE,D3DBLEND_SRCCOLOR,D3DBLEND_INVSRCCOLOR,
 D3DBLEND_SRCALPHA,D3DBLEND_INVSRCALPHA,D3DBLEND_DESTALPHA,D3DBLEND_INVDESTALPHA,
 D3DBLEND_DESTCOLOR,D3DBLEND_INVDESTCOLOR,D3DBLEND_SRCALPHASAT};
#define LOG_DXERR(args) do { if(Vid::dxError)throw std::runtime_error("Vid state device failure"); } while(false)
#define DEVELOPMENT 1
#define ERR_FATAL(args) throw std::runtime_error("Invalid original Vid texture blend flags")
namespace Vid {
 struct PortableRenderState {
  struct {Bool zbuffer=0,wbuffer=0,alpha=0,texStaged=0,texMulti=0;} status;
  U32 renderFlags=0,fogColor=0;
 };
 extern thread_local PortableRenderState renderState;
 extern thread_local reign::VidDevice *device;
 extern thread_local int dxError;
 extern thread_local U32 indexCount;
}
namespace reign {
// Original Vid globals are confined to one bound render-thread scope.
class VidBinding {
 VidDevice *previous_;
 Vid::PortableRenderState previous_state_;
 int previous_error_;
public:
 explicit VidBinding(VidDevice &);
 ~VidBinding();
 VidBinding(const VidBinding&)=delete;
 VidBinding &operator=(const VidBinding&)=delete;
};
}
