#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include "reign/control_prelude.hpp"
namespace reign {
// Persistent scrollback and hooks for controls that survive across frames.
class ConsoleSession {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 using Display=std::function<void(VarSys::VarItem *,std::uint32_t)>;
 explicit ConsoleSession(Display);~ConsoleSession();
 ConsoleSession(const ConsoleSession&)=delete;
 ConsoleSession &operator=(const ConsoleSession&)=delete;
 void run(const std::function<void()> &);
};
}
