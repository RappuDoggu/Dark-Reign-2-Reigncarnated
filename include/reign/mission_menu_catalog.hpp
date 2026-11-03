#pragma once
#include <filesystem>
#include <string>
#include <vector>
namespace reign {
struct MenuMission {std::string group,name;};
// Read original group metadata through PTree, preserving campaign Add() order.
std::vector<MenuMission> menu_missions(const std::filesystem::path &,bool campaign,const std::filesystem::path &progressFile={});
}
