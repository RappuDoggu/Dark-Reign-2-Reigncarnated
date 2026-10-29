#pragma once
#include <functional>
#include <memory>
namespace reign {
class EventLoop;
// Owns original focus/capture/modal state; may borrow the engine event queue.
// This does not initialize IFace's control tree, renderer, fonts or input.
// Use on one owning thread. Destroy controls and stop producers before this
// session; a borrowed EventLoop must outlive it. Other event types are preserved.
class InterfaceEvents {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 InterfaceEvents();explicit InterfaceEvents(EventLoop &);~InterfaceEvents();
 InterfaceEvents(const InterfaceEvents&)=delete;
 InterfaceEvents &operator=(const InterfaceEvents&)=delete;
 void run(const std::function<void()> &);
 void process();
 // Includes other event types when borrowing the engine queue.
 bool empty()const;
};
}
