#include "reign/frontend_presenter.hpp"
#include "reign/frontend_draw.hpp"
#include "reign/frontend_bitmap.hpp"
#include "reign/native_font.hpp"
#include <map>
#include <algorithm>
namespace reign {
struct FrontendPresenter::Impl {
 LegacyGles &renderer;unsigned width=0,height=0;bool open=false;
 struct Entry {unsigned id;std::weak_ptr<const void> owner;};
 std::map<const Bitmap *,Entry> images;
 std::map<std::pair<const NativeFont *,unsigned>,Entry> fonts;
 std::size_t bytes=0;
 std::vector<unsigned> handles;
 explicit Impl(LegacyGles &r):renderer(r){}
 void require(){if(!open)throw std::logic_error("UI frame not open");}
 void release(){for(auto id:handles)renderer.delete_texture(id);handles.clear();images.clear();fonts.clear();bytes=0;}
 void expire(){
  auto sweep=[&](auto &cache){for(auto it=cache.begin();it!=cache.end();){if(it->second.owner.expired()){renderer.delete_texture(it->second.id);handles.erase(std::remove(handles.begin(),handles.end(),it->second.id),handles.end());it=cache.erase(it);}else ++it;}};
  sweep(images);sweep(fonts);if(handles.empty())bytes=0;
 }
 unsigned upload(const RgbaImage &image){if(handles.size()>=512||bytes+image.pixels.size()>64*1024*1024)release();bytes+=image.pixels.size();auto id=renderer.texture_rgba(image.width,image.height,image.pixels.data());try{handles.push_back(id);}catch(...){renderer.delete_texture(id);throw;}return id;}
 void geometry(const FrontendBatch &batch){
  require();DrawState state;state.blend=Blend::alpha;state.filtered=batch.filtered;
  if(batch.texture){auto it=images.find(batch.texture);if(it!=images.end()&&it->second.owner.expired()){expire();it=images.end();}if(it==images.end())it=images.emplace(batch.texture,Entry{upload(batch.texture->surface().pixels()),batch.texture->cache_owner()}).first;state.texture=it->second.id;state.texture_op=TextureOp::modulate_rgba;state.address=batch.clamped?TextureAddress::clamp:TextureAddress::wrap;}
  renderer.draw(width,height,batch.vertices.data(),batch.vertices.size(),batch.indices.data(),batch.indices.size(),state);
 }
};
FrontendPresenter::FrontendPresenter(LegacyGles &r):impl_(std::make_unique<Impl>(r)){}
FrontendPresenter::~FrontendPresenter(){impl_->release();}
void FrontendPresenter::begin(unsigned width,unsigned height){
 if(!width||!height||impl_->open)throw std::invalid_argument("Invalid UI frame begin");
 impl_->expire();impl_->width=width;impl_->height=height;impl_->renderer.clear(width,height,0,0,0);impl_->open=true;
}
void FrontendPresenter::draw(const std::function<void()> &draw){
 impl_->require();try{FrontendDraw::run_batches(draw,[&](const FrontendBatch &b){impl_->geometry(b);});}catch(...){cancel();throw;}
}
void FrontendPresenter::submit(const FrontendBatch &batch){impl_->geometry(batch);}
void FrontendPresenter::submit(const NativeFont &font,const FontBatch &batch){
 impl_->require();auto key=std::make_pair(&font,batch.atlas);auto it=impl_->fonts.find(key);if(it!=impl_->fonts.end()&&it->second.owner.expired()){impl_->expire();it=impl_->fonts.end();}
 if(it==impl_->fonts.end())it=impl_->fonts.emplace(key,Impl::Entry{impl_->upload(font.atlas(batch.atlas)),font.cache_owner()}).first;
 DrawState state;state.blend=Blend::alpha;state.texture_op=TextureOp::modulate_rgba;state.texture=it->second.id;state.filtered=batch.filtered;
 impl_->renderer.draw(impl_->width,impl_->height,batch.vertices.data(),batch.vertices.size(),batch.indices.data(),batch.indices.size(),state);
}
void FrontendPresenter::present(){impl_->require();impl_->renderer.present();impl_->open=false;}
void FrontendPresenter::present(unsigned width,unsigned height){impl_->require();impl_->renderer.present_to(width,height);impl_->open=false;}
void FrontendPresenter::cancel(){impl_->open=false;impl_->release();}
void FrontendPresenter::abandon_context()noexcept{impl_->open=false;impl_->handles.clear();impl_->images.clear();impl_->fonts.clear();impl_->bytes=0;}
}
