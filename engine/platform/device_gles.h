#pragma once
#include "surface_native.h"
#include "d3d.h"
namespace reign {class LegacyGles;}
namespace NativeEngine {
// Render-thread only. Renderer and its current context must outlive this device.
// Initial adapter supports full-target TL triangle lists and one texture stage.
// Unsupported DX7 operations return E_NOTIMPL rather than accepting lost state.
HRESULT CreateGlesDevice(reign::LegacyGles &,IDirectDrawSurface7 *,IDirect3DDevice7 **);
}
