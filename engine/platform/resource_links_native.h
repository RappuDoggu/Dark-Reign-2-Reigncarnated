#pragma once
#include <filesystem>
#include <map>
#include <stdexcept>
#include <vector>

namespace NativeEngine {
// Only app-created resource links change. Profiles and real files are never removed.
inline void PrepareResourceLinks(const std::filesystem::path &root,
                                 const std::filesystem::path &work,
                                 const std::filesystem::path &basePack) {
 namespace fs=std::filesystem;
 struct Change {fs::path link,target;bool directory;};
 std::vector<Change> changes;
 auto plan=[&](const fs::path &link,const fs::path &target,bool directory){
  const auto status=fs::symlink_status(link);
  if(fs::exists(status)&&!fs::is_symlink(status))
   throw std::runtime_error("Cannot replace a real runtime resource: "+link.string());
  if(fs::is_symlink(status)&&!target.empty()&&fs::read_symlink(link)==target)return;
  if(fs::is_symlink(status)||!target.empty())changes.push_back({link,target,directory});
 };
 for(const char *name:{"missions","worlds","sides","mods","library","movies","data"})
  plan(work/name,fs::is_directory(root/name)?root/name:fs::path{},true);
 const auto packs=work/"packs";
 const auto packStatus=fs::symlink_status(packs);
 if(fs::is_symlink(packStatus)||(fs::exists(packStatus)&&!fs::is_directory(packStatus)))
  throw std::runtime_error("Runtime packs must be a real directory");
 std::map<fs::path,fs::path> targets;
 if(fs::is_directory(root/"packs"))for(const auto &entry:fs::directory_iterator(root/"packs"))
  targets[entry.path().filename()]=entry.path();
 targets["base.zwp"]=basePack;
 for(const auto &entry:targets)plan(packs/entry.first,entry.second,false);
 if(fs::is_directory(packs))for(const auto &entry:fs::directory_iterator(packs))
  if(fs::is_symlink(entry.symlink_status())&&!targets.count(entry.path().filename()))plan(entry.path(),{},false);
 // Preflight all conflicts before altering any old link.
 fs::create_directories(packs);
 for(const auto &change:changes){
  if(fs::is_symlink(fs::symlink_status(change.link)))fs::remove(change.link);
  if(!change.target.empty()){
   if(change.directory)fs::create_directory_symlink(change.target,change.link);
   else fs::create_symlink(change.target,change.link);
  }
 }
}
}
