#include "reign/native_font.hpp"
#include "font_slice.hpp"
#include <algorithm>
#include <limits>
namespace reign::font_detail {
thread_local Context *active=nullptr;
Context::~Context()=default;
Context &context(){if(!active)throw std::runtime_error("Original font session not bound");return *active;}
Binding::Binding(Context &c):previous(active){if(active&&active->drawing)throw std::runtime_error("Font draw consumer cannot reenter font processing");active=&c;}
Binding::~Binding(){active=previous;}
Bitmap::Bitmap(U32 flags):BitmapSurface([owner=&context(),index=unsigned(context().currTex-1)](const RgbaImage&){owner->published.at(index)=true;},context().half_texel),handle(context().currTex-1){
 if(flags!=(bitmapTEXTURE|bitmapNOMANAGE))throw std::runtime_error("Unsupported original font bitmap flags");
}
namespace Vid {
thread_local State renderState;
U16 rectIndices[12]={0,1,2,0,2,3,0,3,4,0,4,1};BackBitmap backBmp;
const ClipRect &BackBitmap::GetClipRect()const{return context().options.clip;}
U32 SetFilterStateI(U32 filter){if(filter>1)throw std::runtime_error("Unsupported font filter state");auto &c=context();auto previous=c.filter;c.filter=filter;return previous;}
void SetTexture(Bitmap *texture){context().current_texture=texture;}
void SetMaterial(void *material){if(material)throw std::runtime_error("Font TL path does not implement materials");}
void DrawIndexedPrimitive(U32 type,U32 fvf,VertexTL *vertices,U32 count,U16 *indices,U32 n,U32 flags){
 auto &c=context();if(type!=PT_TRIANGLELIST||fvf!=FVF_TLVERTEX||!c.sink||!c.current_texture||count>65536||!count||!n||n%3)throw std::runtime_error("Invalid font draw submission");
 for(U32 i=0;i<n;++i)if(indices[i]>=count)throw std::runtime_error("Font draw index out of bounds");
 FontBatch batch;batch.atlas=c.current_texture->handle;batch.filtered=c.filter!=0;batch.flags=flags;
 if(!c.published.at(batch.atlas))throw std::runtime_error("Font atlas not published");
 batch.vertices.assign(vertices,vertices+count);batch.indices.assign(indices,indices+n);c.sink(batch);
}
namespace Heap {
U32 ReqVertex(VertexTL **vertices,U16 **indices){auto &c=context();if(!c.vertices.empty()||!c.indices.empty())throw std::runtime_error("Font vertex heap already reserved");
 c.vertices.resize(renderState.maxVerts);c.indices.resize(renderState.maxIndices);*vertices=c.vertices.data();*indices=c.indices.data();return renderState.maxVerts*sizeof(VertexTL)+renderState.maxIndices*sizeof(U16);}
void Restore(U32 size){auto &c=context();if(size!=c.vertices.size()*sizeof(VertexTL)+c.indices.size()*sizeof(U16))throw std::runtime_error("Invalid font heap release");c.vertices.clear();c.indices.clear();}
}
}
namespace IFace {S32 GetMetric(U32 metric){if(metric!=SHADOW_ALPHA)throw std::runtime_error("Unsupported font interface metric");return context().options.shadow_alpha;}}
}
namespace reign {
namespace {
struct FontInfo {unsigned version,count;};
FontInfo validate_font(const std::vector<U8> &file){
 if(file.size()<4||file.size()>128*1024*1024)throw std::runtime_error("Truncated/oversize original font");
 std::size_t at=0;auto take=[&](std::size_t count){if(count>file.size()-at)throw std::runtime_error("Truncated original font field/pixels");auto p=file.data()+at;at+=count;return p;};
 auto magic=take(4);const auto id=font_detail::load32(magic);unsigned version=0,num=0;
 if(id==0x544e4f46u){version=1;num=*take(1);}else if(id==0x4b524f42u){version=2;num=font_detail::load16(take(2));}else throw std::runtime_error("Unknown original font identifier");
 take(3);take(std::size_t(num)*(version==1?6:7));
 for(unsigned i=0;i<num;++i){auto p=take(2);if(p[0]>100||p[1]>100)throw std::runtime_error("Original font glyph exceeds 100-pixel limit");take(std::size_t(p[0])*p[1]);}
 return {version,num};
}
std::vector<CH> characters(const std::u16string &s){if(s.size()>65535)throw std::runtime_error("Font string exceeds diagnostic limit");return {s.begin(),s.end()};}
}
struct NativeFont::Impl {
 font_detail::Context context;font_detail::Font font;FontInfo info;
 Impl(std::vector<U8> file,bool half):info(validate_font(file)){
  context.half_texel=half;font_detail::Binding binding(context);
  FileSys::Session files([&](const std::string &name){if(name!="native-font.fff")throw FileSys::FileNotFound(name);return file;});
  if(!font.Read("native-font.fff"))throw std::runtime_error("Original font read failed");
  for(unsigned i=0;i<context.currTex;++i)if(!context.published[i])throw std::runtime_error("Incomplete original font atlas publication");
 }
};
NativeFont::NativeFont(std::vector<U8> file,bool half):impl_(std::make_unique<Impl>(std::move(file),half)){}
NativeFont::~NativeFont()=default;
unsigned NativeFont::version()const{return impl_->info.version;}
unsigned NativeFont::glyph_count()const{return impl_->info.count;}
unsigned NativeFont::height()const{return impl_->font.Height();}
unsigned NativeFont::width(const std::u16string &s)const{auto text=characters(s);return impl_->font.Width(text.data(),text.size());}
unsigned NativeFont::atlas_count()const{return impl_->context.currTex;}
const RgbaImage &NativeFont::atlas(unsigned index)const{if(index>=atlas_count())throw std::runtime_error("Font atlas index out of range");return impl_->context.textures[index]->pixels();}
void NativeFont::upload(const std::function<void(unsigned,const RgbaImage&)> &sink)const{if(!sink)throw std::runtime_error("Font atlas upload consumer is required");for(unsigned i=0;i<atlas_count();++i)sink(i,atlas(i));}
void NativeFont::draw(const std::u16string &s,const FontDrawOptions &options,const std::function<void(const FontBatch&)> &sink)const{
 auto text=characters(s);if(!sink||options.batch_vertices<8||options.batch_vertices>65532||!std::isfinite(options.alpha_scale)||options.alpha_scale<0||options.alpha_scale>1||options.shadow<-16384||options.shadow>16384||options.clip.p1.x<options.clip.p0.x||options.clip.p1.y<options.clip.p0.y)throw std::runtime_error("Invalid native font draw options");
 const std::int64_t extent=std::int64_t(width(s))+512+std::abs(options.shadow);
 if(std::int64_t(options.x)+extent>std::numeric_limits<S32>::max()||std::int64_t(options.x)-extent<std::numeric_limits<S32>::min()||std::int64_t(options.y)+extent>std::numeric_limits<S32>::max()||std::int64_t(options.y)-extent<std::numeric_limits<S32>::min())throw std::runtime_error("Font coordinate overflow");
 auto &c=impl_->context;font_detail::Binding binding(c);c.options=options;c.sink=sink;c.drawing=true;
 font_detail::Vid::renderState={options.batch_vertices,options.batch_vertices*2+1};
 try{font_detail::IFace::InitBuckets();impl_->font.Draw(options.x,options.y,text.data(),text.size(),font_detail::Color(options.argb),&c.options.clip,options.alpha_scale,options.shadow);font_detail::IFace::DoneBuckets();}
 catch(...){font_detail::IFace::AbortBuckets();c.vertices.clear();c.indices.clear();c.sink={};c.drawing=false;throw;}
 c.sink={};c.drawing=false;
}
}
