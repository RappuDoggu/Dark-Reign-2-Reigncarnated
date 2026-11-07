#include "frontend_configuration_slice.hpp"
#include "reign/frontend_configuration.hpp"
#include "reign/command_scopes.hpp"
namespace reign::frontend_configuration_detail {
namespace {thread_local State *active=nullptr;}
State &state(){if(!active)throw std::runtime_error("Frontend configuration not bound");return *active;}
void forward(FScope *scope){auto &s=state();if(!s.service)throw std::runtime_error(std::string("Unported interface service: ")+scope->NameStr());s.service(scope);}
struct Binding{State *previous;explicit Binding(State &s):previous(active){active=&s;}~Binding(){active=previous;}};
}
namespace reign {
struct FrontendConfiguration::Impl {frontend_configuration_detail::State state;};
FrontendConfiguration::FrontendConfiguration(FrontendTheme &theme,FontRegistry &fonts,InterfaceMetrics &metrics,std::function<void(FScope *)> service):impl_(std::make_unique<Impl>()){
 impl_->state.theme=&theme;impl_->state.fonts=&fonts;impl_->state.metrics=&metrics;impl_->state.service=std::move(service);
}
FrontendConfiguration::~FrontendConfiguration()=default;
void FrontendConfiguration::register_scopes(CommandScopes &scopes){scopes.register_scope("ConfigureInterface",IFace::ConfigureNative);scopes.register_scope("CreateControl",IFace::CreateNativeRootControl);}
void FrontendConfiguration::run(const std::function<void()> &call){if(!call)throw std::invalid_argument("Missing configuration callback");frontend_configuration_detail::Binding binding(impl_->state);call();}
void FrontendConfiguration::initialize_language(){IFace::InitializeNativeLanguage();}
unsigned FrontendConfiguration::processed()const{return impl_->state.processed;}
}
