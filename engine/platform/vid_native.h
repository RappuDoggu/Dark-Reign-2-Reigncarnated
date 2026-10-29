#pragma once
#include "dx7/windows.h"
#include "d3d.h"
#include "dx7/errors.h"
namespace NativeEngine {
// Engine/render-thread boundary. Caller supplies a working device and a present
// callback (including EGL swap). References are retained until DetachVideo.
// This configures display data; engine managers/camera still require startup.
using PresentVideo = bool (*)(void *);
HRESULT AttachVideo(IDirect3DDevice7 *, IDirectDrawSurface7 *, PresentVideo, void *);
void DetachVideo();
}
