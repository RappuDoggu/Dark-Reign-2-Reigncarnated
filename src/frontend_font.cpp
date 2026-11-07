#include "reign/frontend_font.hpp"
#include <map>
#include "reign/frontend_support.hpp"
namespace {
struct FontContext {
 reign::FontRegistry &registry;reign::FrontendFonts::Sink sink;
 reign::PixelRect clip{{0,0},{0,0}};std::uint8_t shadow_alpha=0;bool frame_set=false;
 std::map<const reign::NativeFont *,std::unique_ptr<Font>> wrappers;
 Font *find(U32 crc){auto font=registry.find_crc(crc);if(!font)return nullptr;
  auto &entry=wrappers[font.get()];if(!entry)entry=std::make_unique<Font>(font);return entry.get();}
};
thread_local FontContext *active=nullptr;
FontContext &context(){if(!active)throw std::runtime_error("Frontend fonts not bound");return *active;}
std::u16string text(const CH *s,U32 length){
 if(length>65535||(!s&&length))throw std::runtime_error("Invalid frontend font text");
 std::u16string result;result.reserve(length);for(U32 i=0;i<length;++i)result.push_back(char16_t(s[i]));return result;
}
}
Font::Font(std::shared_ptr<const reign::NativeFont> font):native_(std::move(font)){
 if(!native_)throw std::invalid_argument("Missing native font");
}
S32 Font::Height(){return S32(native_->height());}
S32 Font::Width(const CH *s,S32 length){return length<=0?0:S32(native_->width(text(s,U32(length))));}
void Font::Draw(S32 x,S32 y,const CH *s,U32 length,Color color,const ClipRect *clip,F32 alpha,S32 shadow){
 auto &c=context();if(!c.frame_set)throw std::runtime_error("Frontend font frame not configured");
 reign::FontDrawOptions options;options.x=x;options.y=y;options.argb=U32(color);options.alpha_scale=alpha;
 options.shadow=shadow;options.shadow_alpha=c.shadow_alpha;options.clip=c.clip;
 if(clip)options.clip={{clip->p0.x,clip->p0.y},{clip->p1.x,clip->p1.y}};
 native_->draw(text(s,length),options,[&](const reign::FontBatch &batch){auto command=[font=native_,sink=c.sink,batch]{sink(*font,batch);};if(!reign::defer_frontend_submission(command))command();});
}
namespace FontSys {
Font *GetFont(U32 crc){return context().find(crc);}
Font *GetFont(const char *name){if(!name)throw std::invalid_argument("Missing font alias");return GetFont(Crc::CalcStr(name));}
}
namespace reign {
struct FrontendFonts::Impl {FontContext context;Impl(FontRegistry &r,Sink sink):context{r,std::move(sink)}{
 if(!context.sink)throw std::invalid_argument("Missing font draw consumer");
}};
FrontendFonts::FrontendFonts(FontRegistry &r,Sink sink):impl_(std::make_unique<Impl>(r,std::move(sink))){}
FrontendFonts::~FrontendFonts()=default;
void FrontendFonts::run(const std::function<void()> &call){
 if(!call)throw std::invalid_argument("Missing font callback");
 auto *previous=active;active=&impl_->context;
 struct Restore {FontContext *previous;~Restore(){active=previous;}} restore{previous};call();
}
void FrontendFonts::set_frame(PixelRect clip,std::uint8_t shadow_alpha){
 if(clip.p1.x<clip.p0.x||clip.p1.y<clip.p0.y)throw std::invalid_argument("Invalid font viewport");
 impl_->context.clip=clip;impl_->context.shadow_alpha=shadow_alpha;impl_->context.frame_set=true;
}
}
