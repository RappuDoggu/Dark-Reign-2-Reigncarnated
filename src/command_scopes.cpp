#include "command_slice.hpp"
#include "reign/command_scopes.hpp"
namespace reign::command_detail {
namespace {thread_local State *active=nullptr;}
State &state(){if(!active)throw std::runtime_error("Command scopes not bound");return *active;}
void forward(FScope *scope){auto &s=state();if(!s.service)throw std::runtime_error(std::string("Unported root service: ")+scope->NameStr());s.service(scope);}
struct Binding{State *previous;explicit Binding(State &s):previous(active){active=&s;}~Binding(){active=previous;}};
struct Depth{State &s;explicit Depth(State &value):s(value){if(s.depth>=64)throw std::runtime_error("Configuration dispatch recursion limit");++s.depth;}~Depth(){--s.depth;}};
}
namespace reign {
struct CommandScopes::Impl {command_detail::State state;ConfigLoader loader;explicit Impl(ConfigLoader fn):loader(std::move(fn)){if(!loader)throw std::invalid_argument("Missing command configuration loader");}};
CommandScopes::CommandScopes(ConfigLoader loader,std::function<void(FScope *)> service):impl_(std::make_unique<Impl>(std::move(loader))){impl_->state.service=std::move(service);}
CommandScopes::~CommandScopes()=default;
void CommandScopes::register_scope(const char *name,Handler fn){if(!name||!*name||!fn)throw std::invalid_argument("Invalid root scope registration");command_detail::Binding binding(impl_->state);Main::RegisterRootScope(name,fn);}
void CommandScopes::unregister_scope(const char *name,Handler fn){if(!name||!*name||!fn)throw std::invalid_argument("Invalid root scope removal");command_detail::Binding binding(impl_->state);Main::UnregisterRootScope(name,fn);}
void CommandScopes::dispatch(FScope *scope){command_detail::Binding binding(impl_->state);command_detail::Depth depth(impl_->state);Main::Dispatch(scope);}
bool CommandScopes::execute(const std::string &file,bool required){
 command_detail::Binding binding(impl_->state);
 RunCodeHost host(impl_->loader,[this](FScope *s){dispatch(s);});
 return Main::Exec(file.c_str(),Main::ScopeHandler,required)!=0;
}
}
