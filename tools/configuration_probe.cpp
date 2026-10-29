#include "reign/config.hpp"
#include "reign/resources.hpp"
#include <iostream>
#include <numeric>
namespace {
void count(const std::vector<reign::ConfigNode>&v,std::size_t&scopes,std::size_t&args){
 for(auto &n:v){++scopes;args+=n.arguments.size();count(n.children,scopes,args);}
}
}
int main(int argc,char **argv){
 if(argc<2){std::cerr<<"usage: configuration_probe installation-root [mission-pack]\n";return 2;}
 try{
  reign::ResourceStore sources(argv[1]);sources.add_directory("exe",".");
  auto startup=reign::parse_config([&](auto &name){return sources.load("exe",name);},"library\\engine\\startup.cfg");
  std::size_t scopes=0,args=0;count(startup,scopes,args);
  std::cout<<"STARTUP parsed scopes="<<scopes<<" arguments="<<args<<'\n';
  // Mirror the supplied startup stream order explicitly; not a replacement game interpreter.
  sources.add_directory("gamedata","client");
  for(const auto *stream:{"mods","savegame","mission","world","side","search"})sources.add_stream("gamedata",stream);
  if(!sources.add_archive("gamedata","packs/base.zwp"))throw std::runtime_error("Base pack missing");
  if(argc>2&&!sources.add_archive("mission",argv[2]))throw std::runtime_error("Mission pack missing");
  const auto type_names=std::vector<std::string>{"formation.cfg","types_messages.cfg","types_common.cfg","types_physics.cfg","types_traction.cfg"};
  for(auto &name:type_names){
   auto tree=reign::parse_config([&](auto &n){return sources.load("gamedata",n);},name);
   std::size_t n=0,a=0;count(tree,n,a);std::cout<<"CONFIG "<<name<<" scopes="<<n<<" arguments="<<a<<'\n';
  }
  if(argc>2){auto mission=reign::parse_config([&](auto &n){return sources.load("gamedata",n);},"game.cfg");
   std::size_t n=0,a=0;count(mission,n,a);std::cout<<"MISSION_CONFIG scopes="<<n<<" arguments="<<a<<'\n';
   for(const auto &item:mission)if(item.name=="TerrainGroup"&&!item.arguments.empty()){
    auto world=std::get<std::string>(item.arguments[0]);
    for(char &c:world)if(c>='A'&&c<='Z')c+='a'-'A';
    if(!sources.add_archive("world","worlds/"+world+".zwp"))throw std::runtime_error("World pack missing");
   }
   sources.add_archive("side","sides/client/jda.zwp");sources.add_archive("side","sides/client/sprawler.zwp");
   auto types=reign::parse_config([&](auto &name){return sources.load("gamedata",name);},"types.cfg");
   std::size_t ns=0,as=0;count(types,ns,as);std::cout<<"MISSION_TYPES scopes="<<ns<<" arguments="<<as<<'\n';
   for(const auto *file:{"objects.cfg","mission_exec.cfg","strategic.cfg"}){
    auto data=reign::parse_config([&](auto &name){return sources.load("gamedata",name);},file);
    std::size_t ns=0,as=0;count(data,ns,as);std::cout<<"MISSION_DATA "<<file<<" scopes="<<ns<<" arguments="<<as<<'\n';
   }
  }
  std::cout<<"Original configuration parser: PASS; full engine/video/gameplay NOT initialized\n";
  return 0;
 }catch(const std::exception &e){std::cerr<<"CONFIGURATION_FAIL "<<e.what()<<'\n';return 1;}
}
