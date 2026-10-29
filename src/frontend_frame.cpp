#include "reign/frontend_frame.hpp"
#include "reign/frontend_prelude.hpp"
#include <cmath>
#include "cursor.h"
#include "reign/cursors.hpp"
namespace reign::cursor_bridge {thread_local std::function<unsigned(unsigned)> lookup;}
namespace CursorSys {U32 GetStandardCursor(StdCursor id){if(!reign::cursor_bridge::lookup)throw std::runtime_error("Cursor bridge not bound");return reign::cursor_bridge::lookup(unsigned(id));}}
namespace {
struct FrameState {reign::InterfaceMetrics &metrics;reign::InterfaceFrameTime time{};bool time_set=false;bool caret=true;std::int64_t caret_ms=400;};
thread_local FrameState *active=nullptr;
FrameState &state(){if(!active)throw std::runtime_error("Frontend frame not bound");return *active;}
const reign::InterfaceFrameTime &timing(){auto &s=state();if(!s.time_set)throw std::runtime_error("Engine frame time not supplied");return s.time;}
}
namespace IFace {
Bool CaretState(){return state().caret;}
void CaretMoved(){state().caret=true;state().caret_ms=400;}
S32 GetMetric(U32 type){return state().metrics.get_id(type);}
S32 ScreenWidth(){return state().metrics.viewport().width;}
S32 ScreenHeight(){return state().metrics.viewport().height;}
S32 ScreenDepth(){return state().metrics.viewport().depth;}
U32 ElapsedMs(){return timing().elapsed_ms;}
U32 TimeStepMs(){return timing().step_ms;}
F32 TimeStepSec(){return timing().step_seconds;}
}
namespace reign {
struct FrontendFrame::Impl {FrameState state;explicit Impl(InterfaceMetrics &m):state{m}{}};
FrontendFrame::FrontendFrame(InterfaceMetrics &m):impl_(std::make_unique<Impl>(m)){}
FrontendFrame::~FrontendFrame()=default;
void FrontendFrame::run(const std::function<void()> &call){
 if(!call)throw std::invalid_argument("Missing frame callback");
 auto *previous=active;active=&impl_->state;
 struct Restore {FrameState *previous;~Restore(){active=previous;}} restore{previous};call();
}
void FrontendFrame::set_time(InterfaceFrameTime time){
 if(!std::isfinite(time.step_seconds)||time.step_seconds<0)throw std::invalid_argument("Invalid engine frame seconds");
 impl_->state.caret_ms-=time.step_ms;if(impl_->state.caret_ms<0){impl_->state.caret=!impl_->state.caret;impl_->state.caret_ms=400;}
 impl_->state.time=time;impl_->state.time_set=true;
}
}
