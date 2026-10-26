#pragma once
#include <functional>
#include <memory>
namespace reign {
class LegacyGles;class NativeFont;struct FontBatch;struct FrontendBatch;
// Serialized UI submission. Source bitmaps/fonts must live until draw() returns.
// GPU textures are cached within a frame; this correctness-first bridge does
// not claim a persistent GPU cache or measured handheld performance.
class FrontendPresenter {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 explicit FrontendPresenter(LegacyGles &);~FrontendPresenter();
 FrontendPresenter(const FrontendPresenter&)=delete;
 FrontendPresenter &operator=(const FrontendPresenter&)=delete;
 void begin(unsigned width,unsigned height);
 void draw(const std::function<void()> &);
 void submit(const FrontendBatch &);
 void submit(const NativeFont &,const FontBatch &);
 void present();
 void present(unsigned surface_width,unsigned surface_height);
 void cancel();
 // Use after context loss; do not delete old handles in the replacement context.
 void abandon_context() noexcept;
};
}
