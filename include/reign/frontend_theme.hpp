#pragma once
#include <functional>
#include <memory>
class FScope;
namespace reign {
// Owns original color groups/skins. Keep alive until dependent controls die.
class FrontendTheme {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 FrontendTheme();~FrontendTheme();
 FrontendTheme(const FrontendTheme&)=delete;FrontendTheme &operator=(const FrontendTheme&)=delete;
 void run(const std::function<void()> &);
 void configure_colors(FScope *);
 void configure_group(FScope *);
 void configure_skin(FScope *);
};
}
