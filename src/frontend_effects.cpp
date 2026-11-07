#include "frontend_effects_slice.hpp"
#include "reign/frontend_effects.hpp"
#include <cmath>
#include <stdexcept>
namespace reign::effects_detail {
namespace {thread_local State *active=nullptr;}
State &state(){if(!active)throw std::runtime_error("Frontend effects not bound");return *active;}
struct Binding {State *previous;explicit Binding(State &s):previous(active){active=&s;}~Binding(){active=previous;}};
}
namespace reign {
struct FrontendEffects::Impl {effects_detail::State state;};
FrontendEffects::FrontendEffects():impl_(std::make_unique<Impl>()){}
FrontendEffects::~FrontendEffects()=default;
void FrontendEffects::set(std::uint8_t id,float volume,std::function<void(float)> callback){
 if(!callback||!std::isfinite(volume)||volume<0||volume>1)throw std::invalid_argument("Invalid frontend effect");
 impl_->state.table[id]={std::make_shared<effects_detail::Effect>(effects_detail::Effect{std::move(callback)}),volume};
}
void FrontendEffects::configure(FScope *scope,const Loader &loader){
 if(!loader)throw std::invalid_argument("Missing effect loader");
 effects_detail::Binding binding(impl_->state);auto previous=impl_->state.loader;impl_->state.loader=loader;
 struct Restore{effects_detail::State &state;Loader old;~Restore(){state.loader=std::move(old);}} restore{impl_->state,std::move(previous)};
 IFace::Sound::Configure(scope);
}
void FrontendEffects::clear(std::uint8_t id){impl_->state.table[id]={};}
void FrontendEffects::run(const std::function<void()> &call){if(!call)throw std::invalid_argument("Missing effects callback");effects_detail::Binding binding(impl_->state);call();}
}
