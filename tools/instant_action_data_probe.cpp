#include "reign/mission_catalog.hpp"
#include "reign/archive.hpp"
#include "reign/shell_terrain.hpp"
#include "missions.h"
#include <iostream>
#include <set>

// Data preflight only. Deliberately does not spawn substitute units or report
// a mission boot: Game::PreOpenMission/OpenMission remain full-engine work.
int main(int argc, char **argv) {
 try {
  if (argc < 2 || argc > 3)
   throw std::invalid_argument("Usage: instant_action_data_probe installation [mission=2crazed]");
  const std::string mission = argc == 3 ? argv[2] : "2crazed";
  reign::Archive base(std::filesystem::path(argv[1]) / "packs/base.zwp");
  reign::MissionCatalog catalog(argv[1], [&](const std::string &n) { return base.read(n); });
  const auto *group = Missions::FindGroup("missions\\multiplayer");
  if (!group || !group->InstantAction()) throw std::runtime_error("No Instant Action group");
  const auto *selected = group->FindMission(mission.c_str());
  if (!selected) throw std::runtime_error("Requested mission is absent");
  Missions::SetSelected(selected);
  auto loader = catalog.mission_loader("missions\\multiplayer", mission);
  auto game = reign::parse_config(loader, "game.cfg");
  auto objects = reign::parse_config(loader, "objects.cfg");
  unsigned width = 0, height = 0, teams = 0, defaultClients = 0, count = 0;
  std::string terrainGroup, defaultRule;
  for (const auto &n : game) {
   if (n.name == "TerrainGroup") terrainGroup = std::get<std::string>(n.arguments.at(0));
   if (n.name == "DefaultRule") defaultRule = std::get<std::string>(n.arguments.at(0));
   if (n.name == "WorldInfo") for (const auto &c : n.children) {
    if (c.name == "CellMapX") width = std::get<std::int32_t>(c.arguments.at(0));
    if (c.name == "CellMapZ") height = std::get<std::int32_t>(c.arguments.at(0));
   }
   if (n.name == "DefineTeams") for (const auto &t : n.children) if (t.name == "CreateTeam") {
    ++teams;
    for (const auto &c : t.children) if (c.name == "DefaultClient" && std::get<std::int32_t>(c.arguments.at(0))) ++defaultClients;
   }
  }
  std::set<std::int32_t> ids;
  for (const auto &n : objects) if (n.name == "CreateObject") {
   const auto id = std::get<std::int32_t>(n.arguments.at(1));
   if (id <= 0 || !ids.insert(id).second) throw std::runtime_error("Invalid/duplicate object ID");
   ++count;
  }
  if (!width || !height || !teams || defaultClients != 1 || !count || terrainGroup.empty())
   throw std::runtime_error("Incomplete mission definitions");
  auto terrain = reign::load_shell_terrain(loader, width, height);
  std::cout << "Instant Action data preflight: PASS\nMission: " << mission
   << "\nMap: " << width << 'x' << height << "\nTerrain group: " << terrainGroup
   << "\nMap team definitions: " << teams << " (not instantiated players)"
   << "\nDefault clients: " << defaultClients << "\nObject definitions: " << count
   << "\nTerrain cells: " << terrain.cells.size() << "\nDefault rule: " << defaultRule
   << "\nMission boot, no-opponent setup, touch selection and construction: NOT implemented/tested\n";
 } catch (const std::exception &e) {
  std::cerr << "INSTANT_ACTION_DATA_FAIL " << e.what() << '\n'; return 1;
 }
}
