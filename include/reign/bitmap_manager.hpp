#pragma once
#include "reign/config.hpp"
#include "reign/tga.hpp"
#include <memory>
namespace reign {
struct ManagedBitmap {
 std::string source;RgbaImage image{};unsigned texture=0,frame=0,generation=0;bool resident=false;
};
struct BitmapGpuCallbacks {
 std::function<unsigned(const RgbaImage &)> upload;
 // Destruction/unbinding callbacks must not throw; execute on the owning context thread.
 std::function<void(unsigned)> destroy;
 std::function<void()> unbind;
};
// Scoped original manager slice for level-zero, unreduced, stage-zero textures.
// CPU-only mode uses empty GPU callbacks. No mipmap, movie or reduction support yet.
// Serialize all access, including reads of retained views; loaders/callbacks must not reenter.
class BitmapManager {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 using Exists=std::function<bool(const std::string &)>;
 BitmapManager(ConfigLoader,Exists,BitmapGpuCallbacks={});~BitmapManager();
 BitmapManager(const BitmapManager &)=delete;BitmapManager &operator=(const BitmapManager &)=delete;
 std::shared_ptr<const ManagedBitmap> load(const std::string &);
 std::shared_ptr<const ManagedBitmap> find(const std::string &)const;
 std::vector<std::shared_ptr<const ManagedBitmap>> animation(const std::string &)const;
 unsigned count()const;
 void release(); // Original manager release loop, with native texture unbinding/destruction.
 void restore(); // Original mode-change loop; re-read/re-upload only released entries.
 void abandon_context(); // No GPU calls; stale handles are invalidated, then restore in a new context.
 void clear(); // Dispose original tree, invalidate retained views' GPU handles; CPU pixels may survive.
};
}
