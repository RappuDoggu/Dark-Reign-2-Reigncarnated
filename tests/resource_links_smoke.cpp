#include "resource_links_native.h"
#include <chrono>
#include <fstream>
#include <iostream>

namespace fs=std::filesystem;
static void put(const fs::path &p,const char *s){fs::create_directories(p.parent_path());std::ofstream(p)<<s;}
static std::string read(const fs::path &p){std::ifstream in(p);return {std::istreambuf_iterator<char>(in),{}};}
int main(){
 const auto temp=fs::temp_directory_path()/("reign-resource-links-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 struct Cleanup {fs::path path;~Cleanup(){std::error_code ec;fs::remove_all(path,ec);}} cleanup{temp};
 try{
  const auto a=temp/"old",b=temp/"new",work=temp/"mission-runtime";
  put(a/"packs/base.zwp","old base");put(a/"packs/patch.zwp","old patch");put(a/"worlds/sprawl.zwp","old world");put(a/"movies/intro.bik","old movie");
  put(b/"base.zwp","new base");put(b/"worlds/sprawl.zwp","new world");put(b/"missions/multiplayer/map.zwp","new map");put(work/"users/player/data.cfg","private profile");
  NativeEngine::PrepareResourceLinks(a,work,a/"packs/base.zwp");
  if(read(work/"packs/base.zwp")!="old base"||!fs::exists(work/"packs/patch.zwp"))throw std::runtime_error("initial links");
  NativeEngine::PrepareResourceLinks(b,work,b/"base.zwp");
  fs::remove_all(a); // The importer removes the old snapshot after committing.
  if(read(work/"packs/base.zwp")!="new base"||read(work/"worlds/sprawl.zwp")!="new world"||read(work/"missions/multiplayer/map.zwp")!="new map")throw std::runtime_error("reimport did not update links");
  if(fs::is_symlink(fs::symlink_status(work/"movies"))||fs::is_symlink(fs::symlink_status(work/"packs/patch.zwp")))throw std::runtime_error("stale optional links retained");
  NativeEngine::PrepareResourceLinks(b,work,b/"base.zwp");
  fs::remove(work/"worlds");fs::create_directory_symlink(a/"worlds",work/"worlds");
  NativeEngine::PrepareResourceLinks(b,work,b/"base.zwp");
  if(read(work/"worlds/sprawl.zwp")!="new world")throw std::runtime_error("broken link not replaced");
  fs::remove(work/"missions");put(work/"missions/local.cfg","retain real data");
  bool rejected=false;try{NativeEngine::PrepareResourceLinks(b,work,b/"base.zwp");}catch(const std::runtime_error &){rejected=true;}
  if(!rejected||read(work/"missions/local.cfg")!="retain real data"||read(work/"users/player/data.cfg")!="private profile")throw std::runtime_error("real files/profile not protected");
  fs::remove_all(work/"missions");fs::remove(work/"packs/base.zwp");put(work/"packs/base.zwp","retain real pack");
  rejected=false;try{NativeEngine::PrepareResourceLinks(b,work,b/"base.zwp");}catch(const std::runtime_error &){rejected=true;}
  if(!rejected||read(work/"packs/base.zwp")!="retain real pack"||fs::exists(work/"missions"))throw std::runtime_error("pack conflict preflight failed");
  std::cout<<"Resource links: reimport, flat base, deleted snapshot, stale packs, dangling links, idempotence and private-file protection PASS\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
