#pragma once
#include <cstdint>
#include <filesystem>
#include <vector>
namespace reign {
std::uint64_t monotonic_ns() noexcept;
// Input must be a relative, lexical path inside an existing asset root.
// Symlink escapes are rejected; only regular files under the root are read.
std::vector<std::uint8_t> read_asset(const std::filesystem::path &root,
 const std::filesystem::path &relative, std::uintmax_t limit = 64 * 1024 * 1024);
enum class State { stopped, paused, running };
class Lifecycle {
 bool resumed_ = false, surface_ = false, destroyed_ = false;
public:
 void resume() noexcept { if (!destroyed_) resumed_ = true; }
 void pause() noexcept { resumed_ = false; }
 void surface(bool available) noexcept { surface_ = available; }
 void destroy() noexcept { destroyed_ = true; resumed_ = surface_ = false; }
 State state() const noexcept {
  return destroyed_ ? State::stopped : (resumed_ && surface_ ? State::running : State::paused);
 }
};
}
