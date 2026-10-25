#pragma once
#include "reign/interface_metrics.hpp"
#include <functional>
#include <memory>
namespace reign {
// Values supplied by the engine clock, preserving the original U32 counters.
// There is no independent UI clock or implicit frame advancement.
struct InterfaceFrameTime {
 std::uint32_t elapsed_ms,step_ms;
 float step_seconds;
};
class FrontendFrame {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 explicit FrontendFrame(InterfaceMetrics &);~FrontendFrame();
 FrontendFrame(const FrontendFrame&)=delete;
 FrontendFrame &operator=(const FrontendFrame&)=delete;
 // Metrics must outlive this adapter; serialize access on the UI thread.
 void run(const std::function<void()> &);
 void set_time(InterfaceFrameTime);
};
}
