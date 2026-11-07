#include "surface_gles.h"
#include "reign/legacy_gles.hpp"
#include <stdexcept>
namespace NativeEngine {
SurfaceTexture::SurfaceTexture(IDirectDrawSurface7 *s):surface(s){if(!s)throw std::invalid_argument("Null native texture surface");surface->AddRef();}
SurfaceTexture::~SurfaceTexture(){ReleaseGpu();surface->Release();}
unsigned SurfaceTexture::Refresh(reign::LegacyGles &next){
 if(renderer&&renderer!=&next)throw std::logic_error("Release or abandon the previous texture context first");
 DWORD version=0;if(FAILED(surface->GetUniquenessValue(&version)))throw std::runtime_error("Cannot query surface revision");
 if(texture&&version==uploadedRevision)return texture;
 std::vector<std::uint8_t> rgba;DWORD width=0,height=0;
 if(FAILED(ReadSurfaceRGBA(surface,rgba,width,height,version)))throw std::runtime_error("Cannot read a locked or unsupported native surface");
 if(texture&&width==uploadedWidth&&height==uploadedHeight)next.update_texture_rgba(texture,width,height,rgba.data());
 else {
  const auto replacement=next.texture_rgba(width,height,rgba.data());
  if(texture)renderer->delete_texture(texture);
  texture=replacement;
 }
 renderer=&next;uploadedWidth=width;uploadedHeight=height;uploadedRevision=version;return texture;
}
void SurfaceTexture::ReleaseGpu(){if(texture&&renderer)renderer->delete_texture(texture);AbandonContext();}
void SurfaceTexture::AbandonContext()noexcept{texture=0;renderer=nullptr;uploadedRevision=uploadedWidth=uploadedHeight=0;}
}
