#pragma once
#include "reign/config.hpp"
#include <filesystem>
#include <functional>
#include <memory>
namespace reign {
// Original campaign/difficulty/mission globals admit one owning session at a time.
// This read-only catalog does not initialize or run mission simulation.
class MissionCatalog {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 MissionCatalog(const std::filesystem::path &installation,ConfigLoader fallback);
 ~MissionCatalog();
 MissionCatalog(const MissionCatalog&)=delete;
 MissionCatalog &operator=(const MissionCatalog&)=delete;
 void run(const std::function<void()> &);
 // Read-only assets from a registered mission directory or ZWP archive.
 ConfigLoader mission_loader(const std::string &group,const std::string &mission)const;
};
}
