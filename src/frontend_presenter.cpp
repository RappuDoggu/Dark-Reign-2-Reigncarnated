#include "reign/frontend_presenter.hpp"
#include "reign/frontend_draw.hpp"
#include "reign/frontend_bitmap.hpp"
#include "reign/native_font.hpp"
#include <map>
namespace reign {
struct FrontendPresenter::Impl {
 LegacyGles &renderer;unsigned width=0,height=0;bool open=false;
 std::map<const Bitmap *,unsigned> images;
 std::map<std::pair<const NativeFont *,unsigned>,unsigned> fonts;
 std::vector<unsigned> handles;
 explicit Impl(LegacyGles &r):renderer(r){}
 void require(){if(!open)throw std::logic_error("UI frame not open");}
 void release(){for(auto id:handles)renderer.delete_texture(id);handles.clear();images.clear();fonts.clear();}
 unsigned upload(const RgbaImage &image){auto id=renderer.texture_rgba(image.width,image.height,image.pixels.data());try{handles.push_back(id);}catch(...){renderer.delete_texture(id);throw;}return id;}
 void geometry(const FrontendBatch &batch){
  require();DrawState state;state.blend=Blend::alpha;state.filtered=batch.filtered;
  if(batch.texture){auto it=images.find(batch.texture);if(it==images.end())it=images.emplace(batch.texture,upload(batch.texture->surface().pixels())).first;state.texture=it->second;state.texture_op=TextureOp::modulate_rgba;state.address=batch.clamped?TextureAddress::clamp:TextureAddress::wrap;}
  renderer.draw(width,height,batch.vertices.data(),batch.vertices.size(),batch.indices.data(),batch.indices.size(),state);
 }
};
FrontendPresenter::FrontendPresenter(LegacyGles &r):impl_(std::make_unique<Impl>(r)){}
FrontendPresenter::~FrontendPresenter(){impl_->release();}
void FrontendPresenter::begin(unsigned width,unsigned height){
 if(!width||!height||impl_->open)throw std::invalid_argument("Invalid UI frame begin");
 impl_->release();impl_->width=width;impl_->height=height;impl_->renderer.clear(width,height,0,0,0);impl_->open=true;
}
void FrontendPresenter::draw(const std::function<void()> &draw){
 impl_->require();try{FrontendDraw::run_batches(draw,[&](const FrontendBatch &b){impl_->geometry(b);});}catch(...){cancel();throw;}
}
void FrontendPresenter::submit(const FrontendBatch &batch){impl_->geometry(batch);}
void FrontendPresenter::submit(const NativeFont &font,const FontBatch &batch){
 impl_->require();auto key=std::make_pair(&font,batch.atlas);auto it=impl_->fonts.find(key);
 if(it==impl_->fonts.end())it=impl_->fonts.emplace(key,impl_->upload(font.atlas(batch.atlas))).first;
 DrawState state;state.blend=Blend::alpha;state.texture_op=TextureOp::modulate_rgba;state.texture=it->second;state.filtered=batch.filtered;
 impl_->renderer.draw(impl_->width,impl_->height,batch.vertices.data(),batch.vertices.size(),batch.indices.data(),batch.indices.size(),state);
}
void FrontendPresenter::present(){impl_->require();impl_->renderer.present();impl_->open=false;}
void FrontendPresenter::present(unsigned width,unsigned height){impl_->require();impl_->renderer.present_to(width,height);impl_->open=false;}
void FrontendPresenter::cancel(){impl_->open=false;impl_->release();}
void FrontendPresenter::abandon_context()noexcept{impl_->open=false;impl_->handles.clear();impl_->images.clear();impl_->fonts.clear();}
}
