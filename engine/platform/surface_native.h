#pragma once
#include "dx7/windows.h"
#include "ddraw.h"
#include <vector>
#include <cstdint>
namespace NativeEngine {
// CPU-backed DX7 source interface. All surface operations run on the engine
// thread; reference counting may cross threads. No GPU or window is implied.
HRESULT CreateMemorySurface(const DDSURFACEDESC2 &,IDirectDrawSurface7 **);
HRESULT ReadSurfaceRGBA(IDirectDrawSurface7 *,std::vector<std::uint8_t> &,DWORD &width,DWORD &height,DWORD &revision);
}
