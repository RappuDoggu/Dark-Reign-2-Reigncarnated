#include "theme_slice.hpp"
#include "reign/frontend_theme.hpp"
namespace reign::theme_detail {
namespace {thread_local State *active=nullptr;}
State &state(){if(!active)throw std::runtime_error("Frontend theme not bound");return *active;}
struct Binding {State *previous;explicit Binding(State &s):previous(active){active=&s;}~Binding(){active=previous;}};
}
namespace IFace {SystemData &FrontendData(){return reign::theme_detail::state().data;}}
namespace reign {
struct FrontendTheme::Impl {theme_detail::State state;};
FrontendTheme::FrontendTheme():impl_(std::make_unique<Impl>()){run([]{IFace::InitColors();});}
FrontendTheme::~FrontendTheme()=default;
void FrontendTheme::run(const std::function<void()> &call){if(!call)throw std::invalid_argument("Missing theme callback");theme_detail::Binding binding(impl_->state);call();}
void FrontendTheme::configure_colors(FScope *s){if(!s)throw std::invalid_argument("Missing color scope");run([&]{IFace::ConfigureColors(s);});}
void FrontendTheme::configure_group(FScope *s){if(!s)throw std::invalid_argument("Missing group scope");run([&]{IFace::ProcessCreateColorGroup(s);});}
void FrontendTheme::configure_skin(FScope *s){if(!s)throw std::invalid_argument("Missing skin scope");run([&]{IFace::ProcessCreateSkin(s);});}
}
