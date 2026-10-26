#include "reign/config.hpp"
#include "reign/archive.hpp"
#include "reign/engine_file.hpp"
#include "spline.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
using reign::ConfigNode;
using reign::camera_detail::Vector;
using reign::camera_detail::CubicSpline;
namespace {
const ConfigNode &child(const ConfigNode &node,const std::string &name){for(auto &c:node.children)if(c.name==name)return c;throw std::runtime_error("Missing camera scope: "+name);}
float number(const ConfigNode &n,std::size_t i=0){auto &v=n.arguments.at(i);float result=std::holds_alternative<float>(v)?std::get<float>(v):float(std::get<std::int32_t>(v));if(!std::isfinite(result))throw std::runtime_error("Nonfinite camera value");return result;}
Vector vector(const ConfigNode &n){return {number(n,0),number(n,1),number(n,2)};}
struct Node{Vector position,front;float strength,time;};
std::vector<CubicSpline> curves(const ConfigNode &track){
 if(number(child(track,"Loop"))!=0)throw std::runtime_error("This probe expects a non-looping entry curve");
 std::vector<Node> nodes;for(auto &c:track.children)if(c.name=="Segment"){
  auto &m=child(c,"Matrix");Node n{vector(child(m,"Pos")),vector(child(m,"Front")),number(child(c,"Strength")),number(child(c,"Time"))};if(n.time<=0)throw std::runtime_error("Invalid curve duration");nodes.push_back(n);
 }
 if(nodes.size()<2)throw std::runtime_error("Incomplete camera path");std::vector<CubicSpline> result;
 for(std::size_t i=0;i+1<nodes.size();++i){CubicSpline s;s.SetupHermite(nodes[i].position,nodes[i].front*nodes[i].strength,nodes[i+1].position,nodes[i+1].front*nodes[i+1].strength,nodes[i].time);result.push_back(s);}return result;
}
Vector sample(std::vector<CubicSpline> &curves,float t){for(auto &s:curves){if(t<=s.length)return s.Step(t);t-=s.length;}throw std::runtime_error("Camera sample beyond path duration");}
}
int main(int argc,char **argv){try{
 if(argc!=3)throw std::runtime_error("Usage: shell_camera_probe base.zwp shell.zwp");
 reign::Archive base(argv[1]),shell(argv[2]);auto loader=[&](const std::string &name){auto at=name.find_last_of("/\\");auto key=at==std::string::npos?name:name.substr(at+1);if(shell.find(key))return shell.read(key);if(base.find(key))return base.read(key);throw FileSys::FileNotFound(key);};
 auto objects=reign::parse_config(loader,"objects.cfg");unsigned bookmarks=0;const ConfigNode *entry=nullptr;
 for(auto &object:objects)for(auto &scope:object.children)if(scope.name=="BookmarkObj"){++bookmarks;if(std::get<std::string>(scope.arguments.at(0))=="Main::Enter")entry=&scope;}
 if(!entry)throw std::runtime_error("Main::Enter camera missing");auto &curve=child(*entry,"Curve");auto movement=curves(child(curve,"Movement")),focus=curves(child(curve,"Focus"));
 float duration=0;for(auto &s:movement)duration+=s.length;
 std::cout<<"Shell objects="<<objects.size()<<" bookmarks="<<bookmarks<<" Main::Enter segments="<<movement.size()<<" duration="<<duration<<"s\n";
 for(float t:{0.f,.5f,1.5f,3.f,4.f,5.5f}){auto p=sample(movement,t),f=sample(focus,t);std::cout<<"t="<<t<<" camera="<<p.x<<','<<p.y<<','<<p.z<<" focus="<<f.x<<','<<f.y<<','<<f.z<<'\n';}
 auto terrain=shell.find("terrain.blk");if(!terrain)throw std::runtime_error("Shell terrain missing");std::cout<<"Terrain bytes="<<terrain->size<<"; original camera arithmetic validated. World rendering/cinematic scheduler NOT connected.\n";
}catch(const std::exception &e){std::cerr<<"SHELL_CAMERA_FAIL "<<e.what()<<'\n';return 1;}}
