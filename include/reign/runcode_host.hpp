#pragma once
#include "reign/config.hpp"
#include <memory>
class FScope;
namespace Main {
using ScopeHandlerType=void(FScope *);
void ScopeHandler(FScope *);
void ProcessCmdScope(FScope *,ScopeHandlerType *);
std::int32_t Exec(const char *,ScopeHandlerType *,std::int32_t);
}
namespace reign {
// Hosts the original RunCodes/Main::Exec config hooks; requires real command dispatch.
class RunCodeHost {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 RunCodeHost(ConfigLoader,std::function<void(FScope *)>);
 ~RunCodeHost();
 RunCodeHost(const RunCodeHost&)=delete;
 RunCodeHost &operator=(const RunCodeHost&)=delete;
};
}
