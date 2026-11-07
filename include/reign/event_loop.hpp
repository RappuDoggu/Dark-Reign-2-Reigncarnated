#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
namespace reign {
enum class EventPayload {generic,input,interface};
struct EngineEvent {
 std::uint16_t type=0,subtype=0;
 std::array<std::uint32_t,6> parameters{};
 std::int16_t code=0,character=0;std::uint32_t state=0;std::int32_t x=0,y=0;
 void *to=nullptr,*from=nullptr;std::array<std::uint32_t,4> interface_parameters{};
};
// Register/modify/consume on one thread. Producers may enqueue after registration.
// Native pointer alignment is runtime-only; no packed wire format is defined.
class EventLoop {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 EventLoop();~EventLoop();EventLoop(const EventLoop&)=delete;EventLoop &operator=(const EventLoop&)=delete;
 std::uint16_t register_type(const std::string &,EventPayload=EventPayload::generic);
 void set_handler(std::uint16_t,std::function<bool(const EngineEvent &)>);
 bool put(const EngineEvent &);bool get(EngineEvent &);
 bool empty()const;bool full()const;
 void process();
 // Invoke original engine services (for example IFace::PostEvent) against this
 // loop's native EventSys state. Binding is scoped and restored on exceptions.
 // Run on the loop's owning thread; do not retain references to native state.
 void run_native(const std::function<void()> &);

 // Original FlushEvents ignores type; this clears all pending types.
 void flush(std::uint16_t type);
};
}

