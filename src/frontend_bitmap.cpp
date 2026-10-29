#include "reign/frontend_bitmap.hpp"
#include "reign/bitmap_manager.hpp"
#include <cstring>
#include <map>
#include <tuple>
namespace {
struct TextureContext {
 reign::BitmapManager &manager;
 struct Entry {std::shared_ptr<const reign::ManagedBitmap> owner;std::unique_ptr<Bitmap> bitmap;};
 std::map<std::pair<const reign::ManagedBitmap *,unsigned>,Entry> entries;
 Bitmap *load(const char *name){
  if(!name)throw std::invalid_argument("Missing frontend texture name");
  auto view=manager.load(name);if(!view)return nullptr;
  const auto frames=manager.animation(name);
  if(frames.empty()||frames.front()!=view)throw std::runtime_error("Frontend texture identity could not be resolved");
  if(frames.size()>1)throw std::runtime_error("Animated frontend textures not integrated");
  if(!view->resident)throw std::runtime_error("Frontend texture manager must be restored");
  auto key=std::make_pair(view.get(),view->generation);auto it=entries.find(key);if(it!=entries.end())return it->second.bitmap.get();
  const auto &image=view->image;
  if(!image.width||!image.height||image.pixels.size()!=std::uint64_t(image.width)*image.height*4)
   throw std::runtime_error("Invalid managed frontend image");
  // Publication is performed by FrontendDraw's explicit batch consumer.
  // Do not claim this CPU snapshot uploaded a GPU texture.
  auto surface=std::make_shared<reign::BitmapSurface>([](const auto &){throw std::logic_error("Submit frontend texture through draw consumer");});
  surface->Create(image.width,image.height,true);
  std::memcpy(surface->Lock(),image.pixels.data(),image.pixels.size());surface->UnLock();
  Entry entry{view,std::make_unique<Bitmap>(surface)};
  return entries.emplace(key,std::move(entry)).first->second.bitmap.get();
 }
};
thread_local TextureContext *active=nullptr;
}
Bitmap *Bitmap::Manager::FindCreate(unsigned reduction,const char *name){
 if(reduction!=Bitmap::reduceNONE)throw std::invalid_argument("Frontend texture reduction unsupported");
 if(!active)throw std::runtime_error("Frontend textures not bound");
 return active->load(name);
}
namespace reign {
struct FrontendTextures::Impl {TextureContext context;explicit Impl(BitmapManager &m):context{m}{}};
FrontendTextures::FrontendTextures(BitmapManager &m):impl_(std::make_unique<Impl>(m)){}
FrontendTextures::~FrontendTextures()=default;
void FrontendTextures::run(const std::function<void()> &call){
 if(!call)throw std::invalid_argument("Missing texture callback");
 auto *previous=active;active=&impl_->context;
 struct Restore{TextureContext *previous;~Restore(){active=previous;}} restore{previous};call();
}
}

namespace reign {
void deactivate_control_movie(Bitmap *bitmap) {
 if (!bitmap) throw std::invalid_argument("Null frontend bitmap");
 // This concrete adapter owns only a decoded static CPU image. No movie can
 // be installed in Bitmap; archive movie decoding is rejected by its loader.
 // Therefore the original GetBink()!=nullptr branch is false for this type.
 // A future movie-capable adapter must supply its actual stop operation here.
}
}
