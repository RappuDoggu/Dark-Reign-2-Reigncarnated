#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include "reign/config.hpp"
namespace reign {
struct MenuMission {std::string group,name;};
struct CampaignTestEntry {MenuMission mission;bool cutscene=false,available=false,first=false;};
// Explicit testing catalog: all original Add targets, including missing imports.
std::vector<CampaignTestEntry> menu_campaign_testing(const std::filesystem::path &);
// Read original group metadata through PTree, preserving campaign Add() order.
std::vector<MenuMission> menu_missions(const std::filesystem::path &,bool campaign,const std::filesystem::path &progressFile={});
ConfigLoader menu_mission_loader(const std::filesystem::path &,const MenuMission &);
}
