#pragma once
// Frontend-facing adapter. Glyph parsing/layout/batching remain in NativeFont's
// staged original implementation; this class does not share the legacy ABI.
#define __FONT_H
#include "control_types.hpp"
#include "reign/font_registry.hpp"
class Font {
 std::shared_ptr<const reign::NativeFont> native_;
public:
 explicit Font(std::shared_ptr<const reign::NativeFont>);
 S32 Height();
 S32 Width(const CH *,S32);
 void Draw(S32,S32,const CH *,U32,Color,const ClipRect * = nullptr,F32 = 1.0f,S32 = 0);
 const reign::NativeFont &native()const{return *native_;}
};
namespace FontSys {Font *GetFont(const char *);Font *GetFont(U32);}
namespace reign {
// Registry must outlive the bridge; controls using returned Font* must be
// destroyed first. Serialized calls only. Each batch identifies its font so
// atlas index zero from different fonts cannot alias the same GPU texture.
class FrontendFonts {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 using Sink=std::function<void(const NativeFont &,const FontBatch &)>;
 FrontendFonts(FontRegistry &,Sink);~FrontendFonts();
 FrontendFonts(const FrontendFonts&)=delete;
 FrontendFonts &operator=(const FrontendFonts&)=delete;
 void run(const std::function<void()> &);
 void set_frame(PixelRect clip,std::uint8_t shadow_alpha);
};
}
