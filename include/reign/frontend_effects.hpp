#pragma once
#include <functional>
#include <memory>
#include <cstdint>
#include <string>
class FScope;
namespace reign {
// Original IFace sound routing. Installing an effect requires a real playback
// consumer. Empty entries retain original Init behavior and play nothing.
class FrontendEffects {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 FrontendEffects();~FrontendEffects();
 FrontendEffects(const FrontendEffects&)=delete;
 FrontendEffects &operator=(const FrontendEffects&)=delete;
 void set(std::uint8_t,float,std::function<void(float)>);
 using Loader=std::function<std::function<void(float)>(const std::string &)>;
 void configure(FScope *,const Loader &);
 void clear(std::uint8_t);
 void run(const std::function<void()> &);
};
}
