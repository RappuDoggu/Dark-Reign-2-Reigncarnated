#pragma once
#include "surface_native.h"
namespace reign { class LegacyGles; }
namespace NativeEngine {
// Owns a surface reference and its GLES upload. Use on the render thread.
// Destroy/ReleaseGpu before destroying a live renderer, or AbandonContext when
// the GL context is lost. CPU pixels survive and upload on the next Refresh.
class SurfaceTexture {
 IDirectDrawSurface7 *surface;
 reign::LegacyGles *renderer=nullptr;
 unsigned texture=0;
 DWORD uploadedRevision=0;
public:
 explicit SurfaceTexture(IDirectDrawSurface7 *);
 ~SurfaceTexture();
 SurfaceTexture(const SurfaceTexture &)=delete;
 SurfaceTexture &operator=(const SurfaceTexture &)=delete;
 unsigned Refresh(reign::LegacyGles &);
 void ReleaseGpu();
 void AbandonContext() noexcept;
};
}
