// Included inside namespace Vid by the hash-guarded original-source staging.
namespace {
  NativeEngine::PresentVideo nativePresent = nullptr;
  void *nativePresentContext = nullptr;
  Bool nativeTrilinearOff = TRUE;
}
namespace Config { Bool TrilinearOff() { return nativeTrilinearOff; } }
// Android owns the EGL surface and mode. Desktop options may inspect controls,
// but cannot recreate DirectDraw or switch the Activity's display mode.
Bool InitDD(Bool) { return FALSE; }
Bool SetMode(U32, U32, U32, Bool) { return FALSE; }
Bool RenderFlush()
{
  // An unattached backend is a failure, never a successful invisible frame.
  if (!device || !back || !nativePresent) return FALSE;
  Heap::Check();
  return nativePresent(nativePresentContext) ? TRUE : FALSE;
}
} // namespace Vid

namespace NativeEngine {
void InitializeVideoVariables(){
 if(VarSys::FindVarItem("vid.faroverride"))throw std::logic_error("Video variables already exist");
  VarSys::CreateFloat("vid.faroverride",0,VarSys::DEFAULT,&Vid::Var::farOverride)->SetFloatRange(0.0F,800.0F);
  VarSys::CreateFloat("vid.fogdepth",Vid::renderState.fogDepth,VarSys::DEFAULT,&Vid::Var::fogDepth)->SetFloatRange(0.0F,800.0F);
  VarSys::CreateFloat("vid.farplane",800.0F,VarSys::DEFAULT,&Vid::Var::farPlane)->SetFloatRange(0.0F,800.0F);
  VarSys::CreateInteger("vid.filter",0,VarSys::DEFAULT,&Vid::Var::varFilter);
  VarSys::CreateInteger("vid.mipmap",0,VarSys::DEFAULT,&Vid::Var::varMipmap);
  VarSys::CreateInteger("vid.mipfilter",0,VarSys::DEFAULT,&Vid::Var::varMipfilter);
  VarSys::CreateInteger("vid.fog",0,VarSys::DEFAULT,&Vid::Var::varFog);
  VarSys::CreateInteger("vid.wireframe",0,VarSys::DEFAULT,&Vid::Var::varWire);
  VarSys::CreateInteger("vid.flat",0,VarSys::DEFAULT,&Vid::Var::varFlat);
  VarSys::CreateInteger("vid.specular",Vid::renderState.status.specular,VarSys::DEFAULT,&Vid::Var::varSpecular);
  VarSys::CreateInteger("vid.dither",Vid::renderState.status.dither,VarSys::DEFAULT,&Vid::Var::varDither);
  VarSys::CreateInteger("vid.texture",1,VarSys::DEFAULT,&Vid::Var::varTexture);
  VarSys::CreateInteger("vid.mirror.debug",0,VarSys::DEFAULT,&Vid::Var::mirrorDebug);
  VarSys::CreateInteger("vid.rendpostsim",0,VarSys::DEFAULT,&Vid::Var::varRenderPostSim);
  unsigned perfIndex=0;for(const char *name:{"vid.perf.object","vid.perf.terrain","vid.perf.particle","vid.perf.lighting"})VarSys::CreateFloat(name,1.0f,VarSys::DEFAULT,&Vid::Var::perfs[perfIndex++])->SetFloatRange(0.1f,1.0f);
}

HRESULT AttachGlesVideo(reign::LegacyGles &renderer,unsigned width,unsigned height,PresentVideo present,void *context){
 if(!present||!width||!height||width>16384||height>16384)return E_INVALIDARG;
 if(Vid::device||Vid::back)return E_FAIL;
 DDSURFACEDESC2 desc{};desc.dwSize=sizeof(desc);desc.dwWidth=width;desc.dwHeight=height;
 desc.dwFlags=DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT|DDSD_CAPS;
 desc.ddsCaps.dwCaps=DDSCAPS_SYSTEMMEMORY|DDSCAPS_OFFSCREENPLAIN;
 auto &format=desc.ddpfPixelFormat;format.dwSize=sizeof(format);format.dwFlags=DDPF_RGB|DDPF_ALPHAPIXELS;
 format.dwRGBBitCount=32;format.dwRBitMask=0xff0000;format.dwGBitMask=0xff00;format.dwBBitMask=0xff;format.dwRGBAlphaBitMask=0xff000000;
 IDirectDrawSurface7 *target=nullptr;HRESULT result=CreateMemorySurface(desc,&target);if(FAILED(result))return result;
 IDirect3DDevice7 *device=nullptr;
 result=CreateGlesDevice(renderer,target,&device);
 if(SUCCEEDED(result)){
  try{renderer.clear(width,height,0,0,0,1);result=AttachVideo(device,target,present,context);}catch(...){DetachVideo();result=E_FAIL;}
  device->Release();
 }
 target->Release();return result;
}
void DetachVideo()
{
  using namespace Vid;
  nativeTrilinearOff = TRUE;
  nativePresent = nullptr;
  nativePresentContext = nullptr;
  backBmp.ReleaseDD();
  backBmp.Set(nullptr, 0, 0, 0, 0, 0, nullptr);
  if (back) { back->Release(); back = nullptr; }
  if (device) { device->Release(); device = nullptr; }
  pixFormatList.DisposeAll();
  normalFormat = transparentFormat = translucentFormat = 0;
  backDesc = {};
  backFormat.ClearData();
  viewDesc = {};
  viewRect.Set(0, 0, 0, 0);
  clipRect.Set(0, 0, 0, 0);
  isStatus.active = FALSE;
  isStatus.initialized = FALSE;
  caps.ClearData();
}
HRESULT AttachVideo(IDirect3DDevice7 *next, IDirectDrawSurface7 *target,
                    PresentVideo present, void *context)
{
  using namespace Vid;
  if (!next || !target || !present) return E_INVALIDARG;
  if (device || back) return E_FAIL; // Explicit detach before replacing a backend.
  IDirectDrawSurface7 *actualTarget = nullptr;
  HRESULT targetResult = next->GetRenderTarget(&actualTarget);
  if (FAILED(targetResult)) return targetResult;
  const bool matches = actualTarget == target;
  if (actualTarget) actualTarget->Release();
  if (!matches) return E_INVALIDARG;
  D3DDEVICEDESC7 deviceCaps = {};
  HRESULT capsResult = next->GetCaps(&deviceCaps);
  if (FAILED(capsResult)) return capsResult;
  if (!deviceCaps.dwMaxTextureWidth || !deviceCaps.dwMaxTextureHeight) return E_INVALIDARG;
  DDSURFACEDESC2 desc = {}; desc.dwSize = sizeof(desc);
  HRESULT result = target->GetSurfaceDesc(&desc);
  if (FAILED(result)) return result;
  if (!desc.dwWidth || !desc.dwHeight || desc.dwWidth > 16384 || desc.dwHeight > 16384)
    return E_INVALIDARG;
  if (!(desc.ddpfPixelFormat.dwFlags & DDPF_RGB)) return E_NOTIMPL;

  // Enumerate the actual device formats, validate against the native surface
  // allocator, and publish only once all validation succeeds.
  struct Formats { std::vector<DDPIXELFORMAT> values; HRESULT error = S_OK; } formats;
  result = next->EnumTextureFormats(+[](DDPIXELFORMAT *format, void *opaque)->HRESULT {
    auto &f = *static_cast<Formats *>(opaque);
    if (!format) { f.error = E_INVALIDARG; return D3DENUMRET_CANCEL; }
    DDSURFACEDESC2 test = {}; test.dwSize = sizeof(test);
    test.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
    test.dwWidth = test.dwHeight = 1; test.ddpfPixelFormat = *format;
    test.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    IDirectDrawSurface7 *surface = nullptr;
    if (SUCCEEDED(CreateMemorySurface(test, &surface))) {
      surface->Release();
      try { f.values.push_back(*format); }
      catch (const std::bad_alloc &) { f.error = E_OUTOFMEMORY; return D3DENUMRET_CANCEL; }
    }
    return D3DENUMRET_OK;
  }, &formats);
  if (FAILED(result)) return result;
  if (FAILED(formats.error)) return formats.error;
  if (formats.values.empty()) return E_NOTIMPL;
  D3DVIEWPORT7 viewport = {0, 0, desc.dwWidth, desc.dwHeight, 0.0f, 1.0f};
  result = next->SetViewport(&viewport);
  if (FAILED(result)) return result;
  try {
    for (const auto &format : formats.values) {
      auto pix = std::make_unique<Pix>(); pix->SetPixFmt(format);
      pixFormatList.Append(pix.get()); pix.release();
    }
  } catch (...) { pixFormatList.DisposeAll(); return E_OUTOFMEMORY; }
  normalFormat = transparentFormat = translucentFormat = pixFormatList.GetCount();
  for (U32 i = 0; i < pixFormatList.GetCount(); ++i) {
    const auto &pix = *pixFormatList[i];
    if (!(pix.pixFmt.dwFlags & DDPF_ALPHAPIXELS)) normalFormat = i;
    else if (pix.aScale == 1) transparentFormat = i;
    else translucentFormat = i;
  }
  if (normalFormat == pixFormatList.GetCount()) normalFormat = 0;
  if (translucentFormat == pixFormatList.GetCount()) translucentFormat = normalFormat;
  if (transparentFormat == pixFormatList.GetCount()) transparentFormat = translucentFormat;
  next->AddRef(); target->AddRef(); device = next; back = target;
  backDesc = desc; backFormat.SetPixFmt(desc.ddpfPixelFormat);
  backBmp.Set(back, desc.dwWidth, desc.dwHeight, desc.lPitch,
              desc.ddpfPixelFormat.dwRGBBitCount, desc.ddpfPixelFormat.dwRGBBitCount >> 3, &backFormat);
  backBmp.InitPrimitives(); backBmp.SetName("backbuffer");
  curDD = curMode = 0; numDDs = 1;
  auto &driver = ddDrivers[0]; driver.numModes = driver.numDrivers = 1;
  driver.curMode = driver.curDriver = 0;
  driver.tex32 = FALSE;
  for (const auto &format : formats.values) if (format.dwRGBBitCount == 32) driver.tex32 = TRUE;
  driver.drivers[0].SetDesc(&deviceCaps.deviceGUID, "Native backend", deviceCaps);
  caps.ClearData(); driver.drivers[0].SetCaps();
  // The GLES surface adapter uploads one mip level and advertises no mip filtering.
  renderState.status.filter &= filterFILTER;
  auto &mode = driver.vidModes[0]; mode.desc = desc;
  mode.bpp = desc.ddpfPixelFormat.dwRGBBitCount;
  mode.rect.Set(0, 0, desc.dwWidth, desc.dwHeight);
  viewRect = mode.rect; clipRect.Set(0, 0, float(desc.dwWidth), float(desc.dwHeight));
  viewDesc = viewport;
  currentBucketMan = &bucket;
  // Use the attached backend's DX7 filter capabilities rather than Windows
  // PCI driver-quirk tables. Trilinear requires linear minification and mip blend.
  const DWORD requiredFilters = D3DPTFILTERCAPS_MINFLINEAR | D3DPTFILTERCAPS_MIPFLINEAR;
  nativeTrilinearOff = (deviceCaps.dpcTriCaps.dwTextureFilterCaps & requiredFilters) != requiredFilters;
  nativePresent = present; nativePresentContext = context;
  isStatus.initialized = TRUE;
  isStatus.active = TRUE;
  return S_OK;
}
} // namespace NativeEngine
namespace Vid {
// Android surface mode belongs to the Activity; desktop window toggling fails.
Bool ToggleWindowedMode(){return FALSE;}
