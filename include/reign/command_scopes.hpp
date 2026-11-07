#pragma once
#include "reign/config.hpp"
#include <memory>
class FScope;
namespace reign {
// Original Main conditional/command/Exec/root-scope dispatch. Services not yet
// supplied (key bindings, language/filesystem config, console history) fail.
class CommandScopes {
 struct Impl;std::unique_ptr<Impl> impl_;
 void dispatch(FScope *);
public:
 using Handler=void(*)(FScope *);
 CommandScopes(ConfigLoader,std::function<void(FScope *)> service={});
 ~CommandScopes();
 CommandScopes(const CommandScopes&)=delete;
 CommandScopes &operator=(const CommandScopes&)=delete;
 void register_scope(const char *,Handler);
 void unregister_scope(const char *,Handler);
 bool execute(const std::string &,bool required=true);
};
}
