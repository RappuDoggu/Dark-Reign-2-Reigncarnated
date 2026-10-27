#include "reign/god_mesh.hpp"
#include "reign/archive.hpp"
#include <iostream>
#include <map>
int main(int argc,char **argv){try{
 if(argc<2)throw std::runtime_error("Usage: god_mesh_probe archive.zwp [archive.zwp ...]");
 unsigned models=0;std::size_t vertices=0,faces=0;std::map<unsigned,unsigned> versions;
 for(int i=1;i<argc;++i){reign::Archive pack(argv[i]);for(auto &entry:pack.entries()){
  auto name=entry.name;for(char &c:name)if(c>='A'&&c<='Z')c+=32;
  if(name.size()<4||name.substr(name.size()-4)!=".god")continue;
  try{auto m=reign::read_god_mesh_prefix([&](auto &n){return pack.read(n);},entry.name);
   ++models;++versions[m.version];vertices+=m.vertices.size();faces+=m.faces.size();
   std::cout<<entry.name<<" version="<<m.version<<" vertices="<<m.vertices.size()<<" faces="<<m.faces.size()<<" buckets="<<m.buckets.size()<<" states="<<m.states.size()<<" prefix="<<m.prefixBytes<<'/'<<m.blockBytes<<'\n';
  }catch(const std::exception &e){throw std::runtime_error(entry.name+": "+e.what());}
 }}
 if(!models)throw std::runtime_error("No GOD models found");
 std::cout<<"PASS models="<<models<<" vertices="<<vertices<<" faces="<<faces<<" versions:";for(auto [v,n]:versions)std::cout<<' '<<v<<'='<<n;
 std::cout<<"\nPrefix only: animation, hierarchy, MRM and chunks are not loaded or rendered.\n";
}catch(const std::exception &e){std::cerr<<"GOD_MESH_FAIL "<<e.what()<<'\n';return 1;}}
