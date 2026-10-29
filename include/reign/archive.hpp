#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
namespace reign {
// Original, table-free implementation of the format's documented CRC polynomial.
// Tests compare it against the original engine implementation.
std::uint32_t name_crc(const char *name) noexcept;
struct PackEntry {
 std::string name;
 std::uint32_t directory, offset, size, crc;
};
class Archive {
 std::filesystem::path path_;
 std::vector<PackEntry> entries_;
 std::unordered_map<std::uint32_t, std::size_t> index_;
public:
 explicit Archive(std::filesystem::path path);
 const std::vector<PackEntry> &entries() const noexcept { return entries_; }
 const PackEntry *find(const std::string &filename) const noexcept;
 std::vector<std::uint8_t> read(const std::string &filename, std::uint32_t limit = 64 * 1024 * 1024) const;
};
}
