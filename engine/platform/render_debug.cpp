#include "reign/frame_profile.hpp"
namespace NativeEngine {
thread_local unsigned renderLayers=7;
void SetRenderLayers(unsigned mask){renderLayers=mask&7;}
unsigned RenderLayers(){return renderLayers;}
}
extern "C" {
void __real__ZN7Terrain3Sky6RenderEv();
void __real__ZN10MapObjCtrl6RenderEv();
void __real__ZN6Client7Display6RenderEv();
void __wrap__ZN7Terrain3Sky6RenderEv(){reign::FrameSpan span("Sky");if(NativeEngine::RenderLayers()&1)__real__ZN7Terrain3Sky6RenderEv();}
void __wrap__ZN10MapObjCtrl6RenderEv(){reign::FrameSpan span("Objects");if(NativeEngine::RenderLayers()&2)__real__ZN10MapObjCtrl6RenderEv();}
void __wrap__ZN6Client7Display6RenderEv(){reign::FrameSpan span("HUD");if(NativeEngine::RenderLayers()&4)__real__ZN6Client7Display6RenderEv();}
}
