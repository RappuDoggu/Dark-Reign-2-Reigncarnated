#include "reign/console_session.hpp"
#include "console_slice.hpp"
#include <cmath>
#include <limits>
namespace reign::console_detail {
namespace {thread_local State *active=nullptr;}
State &state(){if(!active)throw std::runtime_error("Console output binding missing");return *active;}
S32 integer(F32 value){
 if(!std::isfinite(value)||double(value)<std::numeric_limits<S32>::min()||double(value)>std::numeric_limits<S32>::max())throw std::range_error("Console integer conversion out of range");
 return static_cast<S32>(value);
}
struct Binding {State *previous;explicit Binding(State &s):previous(active){active=&s;}~Binding(){active=previous;}};
}
namespace Console {void DisplayVarItem(VarSys::VarItem *item,U32 indent){reign::console_detail::state().display(item,indent);}}
namespace reign {
struct ConsoleSession::Impl {console_detail::State state;explicit Impl(Display display):state{std::move(display)}{if(!state.display)throw std::invalid_argument("Console requires output callback");}};
ConsoleSession::ConsoleSession(Display display):impl_(std::make_unique<Impl>(std::move(display))){}
ConsoleSession::~ConsoleSession()=default;
void ConsoleSession::run(const std::function<void()> &call){if(!call)throw std::invalid_argument("Console requires work callback");console_detail::Binding binding(impl_->state);call();}
void run_console(const std::function<void()> &call,const std::function<void(VarSys::VarItem *,U32)> &display){ConsoleSession session(display);session.run(call);}
}

namespace Console {
void RegisterMsgHook(BinTree<U32> *filter,ConsoleHookProc *proc,void *context){
 auto &s=reign::console_detail::state();if(!context||!proc||s.delivering||s.hooks.count(context))throw std::runtime_error("Invalid console hook registration");s.hooks.emplace(context,reign::console_detail::Hook{proc,filter,context});
}
void UnregisterMsgHook(ConsoleHookProc *proc,void *context){
 auto &s=reign::console_detail::state();auto it=s.hooks.find(context);if(s.delivering||it==s.hooks.end()||it->second.proc!=proc)throw std::runtime_error("Invalid console hook removal");s.hooks.erase(it);
}
}
