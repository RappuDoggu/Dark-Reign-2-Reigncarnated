#pragma once
#include <cstdint>
#include <memory>
#include <string>
namespace reign {
class FontRegistry;class NativeFont;
struct InterfaceViewport {std::int32_t width,height,depth;};
// Original IFace metric defaults, CRC lookup, FScope configuration and reads.
// Does not initialize controls, input/events or the full interface.
class InterfaceMetrics {
 struct Impl;std::unique_ptr<Impl> impl_;
 void configure_native(void *scope);friend class FontRegistry;
public:
 InterfaceMetrics(unsigned width,unsigned height);
 ~InterfaceMetrics();
 InterfaceMetrics(const InterfaceMetrics &)=delete;InterfaceMetrics &operator=(const InterfaceMetrics &)=delete;
 std::int32_t get(const std::string &name)const;
 InterfaceViewport viewport()const;
 void resize(unsigned width,unsigned height);
 std::shared_ptr<const NativeFont> font(const std::string &metric,const FontRegistry &)const;
};
}
