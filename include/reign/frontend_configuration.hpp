#pragma once
#include <functional>
#include <memory>
class FScope;
namespace reign {
class FrontendTheme;class FontRegistry;class InterfaceMetrics;class CommandScopes;
class FrontendConfiguration {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 FrontendConfiguration(FrontendTheme &,FontRegistry &,InterfaceMetrics &,std::function<void(FScope *)> service={});
 ~FrontendConfiguration();
 FrontendConfiguration(const FrontendConfiguration&)=delete;
 FrontendConfiguration &operator=(const FrontendConfiguration&)=delete;
 void register_scopes(CommandScopes &);
 void initialize_language();
 void run(const std::function<void()> &);
 unsigned processed()const;
};
}
