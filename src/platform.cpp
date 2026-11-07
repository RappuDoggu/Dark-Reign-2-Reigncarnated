#include "reign/platform.hpp"
#include <chrono>
#include <fstream>
#include <stdexcept>
namespace reign {
std::uint64_t monotonic_ns() noexcept {
 return std::chrono::duration_cast<std::chrono::nanoseconds>(
  std::chrono::steady_clock::now().time_since_epoch()).count();
}
std::vector<std::uint8_t> read_asset(const std::filesystem::path &root,
 const std::filesystem::path &relative, std::uintmax_t limit) {
 if (relative.empty() || relative.is_absolute()) throw std::runtime_error("Invalid asset path");
 for (const auto &part : relative)
  if (part == "..") throw std::runtime_error("Parent traversal rejected");
 const auto base = std::filesystem::canonical(root);
 const auto path = std::filesystem::canonical(base / relative);
 auto bi = base.begin(), pi = path.begin();
 for (; bi != base.end(); ++bi, ++pi)
  if (pi == path.end() || *bi != *pi) throw std::runtime_error("Asset escapes root");
 if (!std::filesystem::is_regular_file(path)) throw std::runtime_error("Not a regular asset");
 const auto size = std::filesystem::file_size(path);
 if (size > limit) throw std::runtime_error("Asset exceeds read limit");
 std::ifstream stream(path, std::ios::binary);
 std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
 if (!stream || (size && !stream.read(reinterpret_cast<char *>(bytes.data()), bytes.size())))
  throw std::runtime_error("Asset read failed");
 return bytes;
}
}
