#pragma once
#include "reign/event_loop.hpp"
#include <memory>
namespace reign {
// Serial menu-thread producer. The caller maps surface coordinates and processes
// the shared queue inside run(), alongside the other frontend bindings.
class FrontendInput {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 explicit FrontendInput(EventLoop &);~FrontendInput();
 FrontendInput(const FrontendInput&)=delete;FrontendInput &operator=(const FrontendInput&)=delete;
 std::uint16_t event_id()const;
 void run(const std::function<void()> &);
 // Queue-full returns false without changing the last accepted pointer state.
 bool move(std::int32_t x,std::int32_t y);
 bool button(unsigned button,bool down,std::int32_t x,std::int32_t y);
};
}
