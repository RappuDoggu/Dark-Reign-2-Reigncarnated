#pragma once
#include <cstdint>
namespace Debug {
namespace Error {
enum Type { MEM, CONFIG, ASSERTION, FATAL, WAIT, MESSAGE };
void Set(const char *file, std::uint32_t line, const char *timestamp, Type type);
[[noreturn]] void Err(const char *format, ...);
}
namespace CallStack { void Dump(); }
namespace Watchdog { void Enable(); void Create(); void Delete(); void Poll(); }
}
