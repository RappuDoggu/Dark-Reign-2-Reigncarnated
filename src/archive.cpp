#include "reign/archive.hpp"
#include <array>
#include <fstream>
#include <stdexcept>
#include <utility>
namespace reign {
std::uint32_t name_crc(const char *name) noexcept {
 std::uint32_t value = ~std::uint32_t{0};
 for (auto p = reinterpret_cast<const unsigned char *>(name); *p; ++p) {
  auto c = *p;
  if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
  value ^= std::uint32_t(c) << 24;
  for (unsigned bit = 0; bit < 8; ++bit)
   value = (value << 1) ^ ((value & 0x80000000u) ? 0x04c11db7u : 0u);
 }
 return ~value;
}
namespace {
std::uint32_t le32(const std::uint8_t *p) {
 return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
        (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}
void exact(std::istream &f, void *out, std::size_t count) {
 if (count && !f.read(static_cast<char *>(out), count)) throw std::runtime_error("Truncated archive read");
}
}
Archive::Archive(std::filesystem::path path) : path_(std::move(path)) {
 std::ifstream f(path_, std::ios::binary);
 if (!f) throw std::runtime_error("Cannot open archive");
 f.seekg(0, std::ios::end);
 const auto end = f.tellg();
 if (end < 56) throw std::runtime_error("Truncated archive header");
 const auto bytes = static_cast<std::uint64_t>(end);
 f.seekg(0);
 std::array<std::uint8_t, 56> h{}; exact(f, h.data(), h.size());
 if (std::string(reinterpret_cast<char *>(h.data()), 4) != "NORK" || le32(h.data()+4) != 1)
  throw std::runtime_error("Unsupported archive signature/version");
 const auto dirs = le32(h.data()+8), doff = le32(h.data()+12);
 const auto count = le32(h.data()+16), foff = le32(h.data()+20);
 if (foff < 56 || doff < foff || doff > bytes || count > (doff-foff)/14 ||
     dirs > (bytes-doff)/2 || count > 1000000 || std::uint64_t(doff)-foff > 64*1024*1024)
  throw std::runtime_error("Invalid archive table ranges/counts");
 std::vector<std::uint8_t> table(doff-foff);
 f.seekg(foff); exact(f, table.data(), table.size());
 std::size_t at = 0;
 auto need = [&](std::size_t n) { if (n > table.size()-at) throw std::runtime_error("Truncated archive record"); };
 entries_.reserve(count);
 for (std::uint32_t i = 0; i < count; ++i) {
  need(5); const auto dir = le32(table.data()+at); at += 4;
  const auto length = table[at++]; need(std::size_t(length)+8);
  std::string name(reinterpret_cast<char *>(table.data()+at), length); at += length;
  const auto offset = le32(table.data()+at), size = le32(table.data()+at+4); at += 8;
  if (name.empty() || name.find_first_of(std::string("/\\\0",3)) != std::string::npos || dir > dirs ||
      offset < 56 || std::uint64_t(offset)+size > foff)
   throw std::runtime_error("Invalid archive asset record");
  const auto crc = name_crc(name.c_str());
  if (!index_.emplace(crc, entries_.size()).second) throw std::runtime_error("Duplicate filename or CRC collision in archive");
  entries_.push_back({name, dir, offset, size, crc});
 }
 // The engine ignores directory names for lookups, but validate their serialized bounds.
 f.seekg(doff);
 std::uint64_t pos = doff;
 for (std::uint32_t i=0; i<dirs; ++i) {
  std::uint8_t length=0; exact(f, &length, 1); ++pos;
  if (!length || std::uint64_t(length) > bytes-pos) throw std::runtime_error("Invalid archive directory record");
  std::array<char,255> name{}; exact(f, name.data(), length); pos += length;
  for (unsigned k=0; k<length; ++k) if (!name[k]) throw std::runtime_error("NUL in directory name");
 }
}
const PackEntry *Archive::find(const std::string &name) const noexcept {
 if (name.find('\0') != std::string::npos) return nullptr;
 const auto it = index_.find(name_crc(name.c_str()));
 return it == index_.end() ? nullptr : &entries_[it->second];
}
std::vector<std::uint8_t> Archive::read(const std::string &name, std::uint32_t limit) const {
 const auto *entry = find(name);
 if (!entry) throw std::runtime_error("Asset not in archive");
 if (entry->size > limit) throw std::runtime_error("Asset exceeds read limit");
 std::ifstream f(path_, std::ios::binary);
 if (!f) throw std::runtime_error("Cannot reopen archive");
 f.seekg(entry->offset);
 if (!f) throw std::runtime_error("Cannot seek archive");
 std::vector<std::uint8_t> out(entry->size); exact(f, out.data(), out.size()); return out;
}
}
