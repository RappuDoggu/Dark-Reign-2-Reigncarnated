#include "reign/god_pose.hpp"
#include "reign/archive.hpp"
#include <iostream>
#include <functional>
int main(int argc,char **argv){try{
 if(argc<2)throw std::runtime_error("Usage: god_pose_probe archive.zwp [archive.zwp ...]");
 unsigned roots=0,cycles=0,samples=0,moving=0;std::size_t vertices=0;
 std::function<void(const reign::GodMesh&)> probe=[&](const reign::GodMesh &m){reign::GodPoseEvaluator evaluator(m);++roots;auto rest=evaluator.sample("");vertices+=rest.vertices.size();
  for(auto &c:m.cycles){++cycles;auto start=evaluator.sample(c.name,0);bool changed=false;for(float f:{c.maxFrame*.25f,c.maxFrame*.5f,c.maxFrame*.75f,c.maxFrame}){auto pose=evaluator.sample(c.name,f);changed|=pose.vertices!=start.vertices;++samples;}moving+=changed;}
  for(auto &chunk:m.chunks)probe(chunk);
 };
 for(int i=1;i<argc;++i){reign::Archive pack(argv[i]);for(auto &e:pack.entries())if(e.name.size()>4&&e.name.substr(e.name.size()-4)==".god"){
  try{auto model=reign::read_god_mesh([&](auto &name){return pack.read(name);},e.name);probe(model);std::cout<<e.name<<" cycles:";for(auto &c:model.cycles)std::cout<<' '<<c.name;std::cout<<'\n';}catch(const std::exception &x){throw std::runtime_error(e.name+": "+x.what());}
 }}
 std::cout<<"PASS roots="<<roots<<" cycles="<<cycles<<" samples="<<samples<<" cycles-with-moving-vertices="<<moving<<" vertices="<<vertices<<'\n';
}catch(const std::exception&e){std::cerr<<"GOD_POSE_FAIL "<<e.what()<<'\n';return 1;}}
