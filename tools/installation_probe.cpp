#include "reign/archive.hpp"
#include "reign/platform.hpp"
#include "reign/tokenizer.hpp"
#include "reign/legacy_types.hpp"
#include "crc.h"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
int main(int argc, char **argv) {
 if (argc != 2) { std::cerr << "usage: installation_probe installation-root\n"; return 2; }
 try {
  const auto root = std::filesystem::canonical(argv[1]);
  std::size_t packs=0, entries=0, cfg_text=0, cfg_binary=0, tokens=0, failures=0;
  std::uint64_t payload_bytes=0;
  std::vector<std::filesystem::path> paths;
  for (const auto &entry : std::filesystem::recursive_directory_iterator(root))
   if (entry.is_regular_file() && entry.path().extension() == ".zwp") paths.push_back(entry.path());
  std::sort(paths.begin(),paths.end());
  for (const auto &path : paths) {
   try {
    reign::Archive archive(path); ++packs;
    for (const auto &entry : archive.entries()) {
     if (entry.crc != Crc::CalcStr(entry.name.c_str())) throw std::runtime_error("Engine CRC mismatch");
     auto upper = entry.name;
     for (char &c : upper) if (c >= 'a' && c <= 'z') c -= 'a'-'A';
     if (archive.find(upper) != &entry) throw std::runtime_error("Case-fold lookup mismatch");
     const auto payload = archive.read(entry.name); ++entries; payload_bytes += payload.size();
     if (std::filesystem::path(entry.name).extension() == ".cfg") {
      if (payload.size() >= 4 && std::string(payload.begin(),payload.begin()+4) == "DOCB") ++cfg_binary;
      else {
       try { tokens += reign::tokenize(std::string_view(reinterpret_cast<const char *>(payload.data()),payload.size()),entry.name).size(); ++cfg_text; }
       catch (const std::exception &e) { ++failures; std::cerr << "TOKEN_FAIL " << path.lexically_relative(root).generic_string() << '/' << entry.name << ": " << e.what() << '\n'; }
      }
     }
    }
    std::cout << "PACK " << path.lexically_relative(root).generic_string() << " entries=" << archive.entries().size() << '\n';
   } catch (const std::exception &e) { ++failures; std::cerr << "PACK_FAIL " << path.lexically_relative(root).generic_string() << ": " << e.what() << '\n'; }
  }
  std::size_t loose_cfg=0,loose_tokens=0;
  for (const auto &file : std::filesystem::recursive_directory_iterator(root)) {
   if (!file.is_regular_file() || file.path().extension() != ".cfg") continue;
   auto relative=file.path().lexically_relative(root);
   if (*relative.begin() == "users") continue; // Do not inspect player profiles/save files.
   const auto bytes = reign::read_asset(root, relative);
   try {
    loose_tokens += reign::tokenize(std::string_view(bytes.empty() ? "" : reinterpret_cast<const char *>(bytes.data()),bytes.size()),file.path().filename().string()).size();
    ++loose_cfg;
   } catch (const std::exception &e) { ++failures; std::cerr << "LOOSE_TOKEN_FAIL " << relative.generic_string() << ": " << e.what() << '\n'; }
  }
  const auto startup = reign::read_asset(root,"library/engine/startup.cfg");
  const auto startup_tokens = reign::tokenize(std::string_view(reinterpret_cast<const char *>(startup.data()),startup.size()),"startup.cfg");
  bool startup_scope=false,game_scope=false;
  for (const auto &t:startup_tokens) { startup_scope |= t.text == "StartupConfig"; game_scope |= t.text == "GameConfig"; }
  if (!startup_scope || !game_scope || paths.empty()) ++failures;
  std::cout << "SUMMARY packs=" << packs << " entries=" << entries << " payload_bytes=" << payload_bytes
   << " text_cfg=" << cfg_text << " binary_cfg=" << cfg_binary << " cfg_tokens=" << tokens
   << " loose_cfg=" << loose_cfg << " loose_tokens=" << loose_tokens << " startup_tokens=" << startup_tokens.size() << " failures=" << failures << '\n';
  return failures ? 1 : 0;
 } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
