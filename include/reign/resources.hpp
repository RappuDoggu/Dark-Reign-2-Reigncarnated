#pragma once
#include "reign/archive.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
namespace reign {
class ResourceStore {
 struct Directory { std::unordered_map<std::uint32_t,std::filesystem::path> files; };
 using Source=std::variant<Directory,std::shared_ptr<Archive>,std::string>;
 std::filesystem::path root_;
 std::unordered_map<std::string,std::filesystem::path> paths_;
 std::unordered_map<std::string,std::vector<Source>> streams_;
 std::vector<std::uint8_t> search(const std::string &,const std::string &,std::vector<std::string> &) const;
 std::filesystem::path resolve(const std::string &) const;
public:
 explicit ResourceStore(std::filesystem::path root);
 bool add_directory(const std::string &stream,const std::string &relative,bool recursive=false);
 bool add_archive(const std::string &stream,const std::string &relative);
 void add_stream(const std::string &stream,const std::string &target);
 std::vector<std::uint8_t> load(const std::string &stream,const std::string &name) const;
};
}
