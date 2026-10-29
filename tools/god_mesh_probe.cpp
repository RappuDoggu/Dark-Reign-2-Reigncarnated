#include "reign/god_mesh.hpp"
#include "reign/archive.hpp"
#include <iostream>
#include <map>
#include <functional>
int main(int argc,char **argv){try{
 if(argc<2)throw std::runtime_error("Usage: god_mesh_probe archive.zwp [archive.zwp ...]");
 std::size_t nodes=0,cycles=0,tracks=0,keys=0,chunks=0,mrms=0;
 std::function<void(const reign::GodMesh&)> tally=[&](const reign::GodMesh&m){nodes+=m.nodes.size();cycles+=m.cycles.size();mrms+=bool(m.mrm);chunks+=m.chunks.size();for(auto &c:m.cycles){tracks+=c.animations.size();for(auto &a:c.animations)keys+=a.keys.size();}for(auto &c:m.chunks)tally(c);};
 unsigned models=0;std::size_t vertices=0,faces=0;std::map<unsigned,unsigned> versions;
 for(int i=1;i<argc;++i){reign::Archive pack(argv[i]);for(auto &entry:pack.entries()){
  auto name=entry.name;for(char &c:name)if(c>='A'&&c<='Z')c+=32;
  if(name.size()<4||name.substr(name.size()-4)!=".god")continue;
  try{auto full=reign::read_god_mesh([&](auto &n){return pack.read(n);},entry.name);auto &m=full.geometry;tally(full);
   ++models;++versions[m.version];vertices+=m.vertices.size();faces+=m.faces.size();
   std::cout<<entry.name<<" version="<<m.version<<" vertices="<<m.vertices.size()<<" faces="<<m.faces.size()<<" buckets="<<m.buckets.size()<<" states="<<m.states.size()<<" nodes="<<full.nodes.size()<<" cycles="<<full.cycles.size()<<" chunks="<<full.chunks.size()<<" prefix="<<m.prefixBytes<<'/'<<m.blockBytes<<'\n';
  }catch(const std::exception &e){throw std::runtime_error(entry.name+": "+e.what());}
 }}
 if(!models)throw std::runtime_error("No GOD models found");
 std::cout<<"PASS models="<<models<<" vertices="<<vertices<<" faces="<<faces<<" versions:";for(auto [v,n]:versions)std::cout<<' '<<v<<'='<<n;
 std::cout<<"\nRecursive totals: nodes="<<nodes<<" cycles="<<cycles<<" tracks="<<tracks<<" keys="<<keys<<" chunks="<<chunks<<" MRM="<<mrms;
 std::cout<<"\nComplete serialized root trees validated. Runtime animation and model rendering are not connected.\n";
}catch(const std::exception &e){std::cerr<<"GOD_MESH_FAIL "<<e.what()<<'\n';return 1;}}
