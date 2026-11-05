#include "reign/mission_menu_catalog.hpp"
#include "reign/config.hpp"
#include "reign/platform.hpp"
#include "reign/archive.hpp"
#include "reign/mission_fs.hpp"
#include <algorithm>
#include <set>
#include <cctype>
namespace reign {
namespace {
std::string lower(std::string s){for(auto &c:s)c=char(std::tolower(static_cast<unsigned char>(c)));return s;}
bool simple(const std::string &s){return !s.empty()&&s!="."&&s!=".."&&s.size()<128&&s.find_first_of("/\\:\t\r\n") == std::string::npos;}
}
std::vector<MenuMission> menu_missions(const std::filesystem::path &root,bool campaign,const std::filesystem::path &progressFile){
 namespace fs=std::filesystem;std::vector<MenuMission> result;const auto missions=root/"missions";if(!fs::is_directory(missions))return result;
 std::set<std::string> completed;
 auto key=[](std::string group,std::string mission){std::replace(group.begin(),group.end(),'\\','/');return lower(std::filesystem::path(group).lexically_normal().generic_string())+"/"+lower(mission);};
 if(campaign&&!progressFile.empty()&&fs::is_regular_file(progressFile)){auto progress=parse_config([&](const std::string &n){return read_asset(progressFile.parent_path(),n);},progressFile.filename().string());for(const auto &rootNode:progress)if(rootNode.name=="MissionProgress")for(const auto &record:rootNode.children)if(record.name=="Mission"&&record.arguments.size()>=2)for(const auto &field:record.children)if(field.name=="Won"&&!field.arguments.empty()&&std::uint32_t(std::get<std::int32_t>(field.arguments[0]))>0)completed.insert(key(std::get<std::string>(record.arguments[0]),std::get<std::string>(record.arguments[1])));}
 std::vector<fs::path> groups;for(const auto &entry:fs::directory_iterator(missions))if(entry.is_directory()&&!entry.is_symlink()&&simple(entry.path().filename().string()))groups.push_back(entry.path());std::sort(groups.begin(),groups.end());
 for(const auto &path:groups){if(!fs::is_regular_file(path/"group.cfg"))continue;auto cfg=parse_config([&](const std::string &n){return read_asset(path,n);},"group.cfg");bool system=false,instant=false;const ConfigNode *sequence=nullptr;
  for(const auto &n:cfg){if(n.name=="System"&&!n.arguments.empty())system=std::get<std::int32_t>(n.arguments[0])!=0;if(n.name=="InstantAction"&&!n.arguments.empty())instant=std::get<std::int32_t>(n.arguments[0])!=0;if(n.name=="Campaign")for(const auto &c:n.children)if(c.name=="Missions")sequence=&c;}
  if(system||(!campaign&&!instant)||(campaign&&!sequence))continue;
  std::set<std::string> available;for(const auto &entry:fs::directory_iterator(path)){if(entry.is_symlink())continue;if(entry.is_directory()&&fs::is_regular_file(entry.path()/"game.cfg"))available.insert(lower(entry.path().filename().string()));else if(entry.is_regular_file()&&lower(entry.path().extension().string())==".zwp")available.insert(lower(entry.path().stem().string()));}
  auto add=[&](const std::string &name){if(simple(name)&&available.count(lower(name)))result.push_back({"missions/"+path.filename().string(),name});};
  if(campaign){bool unlocked=true;std::set<std::string> seen;for(const auto &n:sequence->children)if(n.name=="Add"&&!n.arguments.empty()){const auto &name=std::get<std::string>(n.arguments[0]);if(!seen.insert(lower(name)).second)throw std::runtime_error("Duplicate campaign mission");if(!simple(name))throw std::runtime_error("Invalid campaign mission name");if(unlocked){add(name);if(!completed.count(key("missions/"+path.filename().string(),name)))unlocked=false;}}}
  else for(const auto &name:available)add(name);
 }
 return result;
}
std::vector<CampaignTestEntry> menu_campaign_testing(const std::filesystem::path &root){
 namespace fs=std::filesystem;std::vector<CampaignTestEntry> result;const auto missions=root/"missions";if(!fs::is_directory(missions))return result;
 std::vector<fs::path> groups;for(const auto &entry:fs::directory_iterator(missions))if(entry.is_directory()&&!entry.is_symlink()&&simple(entry.path().filename().string()))groups.push_back(entry.path());std::sort(groups.begin(),groups.end());
 for(const auto &path:groups){
  if(!fs::is_regular_file(path/"group.cfg"))continue;auto cfg=parse_config([&](const std::string &n){return read_asset(path,n);},"group.cfg");bool system=false;const ConfigNode *sequence=nullptr;
  for(const auto &n:cfg){if(n.name=="System"&&!n.arguments.empty())system=std::get<std::int32_t>(n.arguments[0])!=0;if(n.name=="Campaign")for(const auto &c:n.children)if(c.name=="Missions")sequence=&c;}
  if(system||!sequence)continue;
  std::set<std::string> available;for(const auto &entry:fs::directory_iterator(path)){if(entry.is_symlink())continue;if(entry.is_directory()&&fs::is_regular_file(entry.path()/"game.cfg"))available.insert(lower(entry.path().filename().string()));else if(entry.is_regular_file()&&lower(entry.path().extension().string())==".zwp")available.insert(lower(entry.path().stem().string()));}
  std::set<std::string> seen;bool first=true;
  for(const auto &n:sequence->children)if(n.name=="Add"&&!n.arguments.empty()){
   const auto &name=std::get<std::string>(n.arguments[0]);auto folded=lower(name);if(!simple(name))throw std::runtime_error("Invalid campaign mission name");if(!seen.insert(folded).second)throw std::runtime_error("Duplicate campaign mission");
   const bool scene=folded=="intro"||folded=="outro"||(folded.size()>=6&&folded.compare(folded.size()-6,6,"_intro")==0)||folded.rfind("transition into ",0)==0;
   result.push_back({{"missions/"+path.filename().string(),name},scene,available.count(folded)!=0,first});first=false;
  }
 }
 return result;
}
ConfigLoader menu_mission_loader(const std::filesystem::path &root,const MenuMission &map){
 if(!simple(map.name)||map.group.rfind("missions/",0)!=0||!simple(map.group.substr(9)))throw std::invalid_argument("Invalid menu mission resource request");
 std::filesystem::path directory,pack;mission_fs::with_root(root,[&]{directory=mission_fs::resolve_path(map.group+"/"+map.name);pack=mission_fs::resolve_path(map.group+"/"+map.name+".zwp");});
 if(std::filesystem::is_directory(directory))return [directory](const std::string &name){std::vector<std::uint8_t> bytes;mission_fs::with_root(directory,[&]{auto file=mission_fs::resolve_path(name);if(!std::filesystem::is_regular_file(file))throw FileSys::FileNotFound(name);bytes=read_asset(directory,std::filesystem::relative(file,directory));});return bytes;};
 auto archive=std::make_shared<Archive>(pack);return [archive](const std::string &name){if(!archive->find(name))throw FileSys::FileNotFound(name);return archive->read(name);};
}
}
