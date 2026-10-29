#pragma once
#include "surface_native.h"
#include "../../include/reign/legacy_gles.hpp"
#include "d3d.h"
namespace reign {class LegacyGles;}
namespace NativeEngine {
// Render-thread only. Renderer and its current context must outlive this device.
// Adapter supports full-target TL triangle lists/strips/fans and one texture stage.
// Unsupported DX7 operations return E_NOTIMPL rather than accepting lost state.
HRESULT CreateGlesDevice(reign::LegacyGles &,IDirectDrawSurface7 *,IDirect3DDevice7 **);
}
