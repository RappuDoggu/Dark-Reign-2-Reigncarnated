#include "reign/shell_camera.hpp"
#include "bookmark_curve.h"
#include <cmath>
#include <limits>
namespace reign {
namespace {
using camera_detail::BookmarkObj;
using camera_detail::Vector;
const ConfigNode *optional(const ConfigNode &node,const char *name){const ConfigNode *found=nullptr;for(auto &c:node.children)if(c.name==name){if(found)throw std::runtime_error(std::string("Duplicate camera scope: ")+name);found=&c;}return found;}
const ConfigNode &required(const ConfigNode &node,const char *name){auto *found=optional(node,name);if(!found)throw std::runtime_error(std::string("Missing camera scope: ")+name);return *found;}
float number(const ConfigNode &n,std::size_t index=0){if(index>=n.arguments.size())throw std::runtime_error("Missing camera number");auto &v=n.arguments[index];float f;if(auto *i=std::get_if<std::int32_t>(&v))f=float(*i);else if(auto *p=std::get_if<float>(&v))f=*p;else throw std::runtime_error("Camera number is text");if(!std::isfinite(f))throw std::runtime_error("Nonfinite camera number");return f;}
Vector vector(const ConfigNode &n){if(n.arguments.size()!=3)throw std::runtime_error("Camera vector requires three components");return {number(n),number(n,1),number(n,2)};}
struct Track {
 BookmarkObj::CurveSrc source;
 BookmarkObj::Curve curve;
 Track(){curve.loop=0;curve.totalTime=0;}
 ~Track(){source.segmentList.DisposeAll();curve.segmentList.DisposeAll();}
 void load(const ConfigNode &node){
  const float loop=number(required(node,"Loop"));if(loop!=0&&loop!=1)throw std::runtime_error("Invalid camera Loop");
  unsigned count=0;double total=0;
  for(auto &segment:node.children)if(segment.name=="Segment"){
   if(++count>4096)throw std::runtime_error("Too many camera segments");
   const auto &matrix=required(segment,"Matrix");camera_detail::Matrix m{vector(required(matrix,"Pos")),vector(required(matrix,"Front"))};
   float strength=number(required(segment,"Strength")),time=number(required(segment,"Time"));
   if(time<=0||time>86400||std::abs(strength)>1e7f)throw std::runtime_error("Invalid camera segment duration/strength");
   total+=time;if(total>86400)throw std::runtime_error("Camera path duration exceeds one day");
   // Original Build derives Hermite tangents from Front * Strength.
   source.AddNode(m,strength,time);
  }
  if(count<(loop?1u:2u))throw std::runtime_error("Incomplete camera path");
  curve.Build(source,loop!=0);
  if(!std::isfinite(curve.totalTime)||curve.totalTime<=0)throw std::runtime_error("Invalid built camera path");
 }
};
}
struct ShellCamera::Impl {
 std::string name;Track movement;std::unique_ptr<Track> focus;
 explicit Impl(const ConfigNode &bookmark){
  if(bookmark.name!="BookmarkObj"||bookmark.arguments.size()!=1||!std::holds_alternative<std::string>(bookmark.arguments[0]))throw std::runtime_error("Expected named BookmarkObj");
  name=std::get<std::string>(bookmark.arguments[0]);auto &curve=required(bookmark,"Curve");movement.load(required(curve,"Movement"));
  if(auto *f=optional(curve,"Focus")){focus=std::make_unique<Track>();focus->load(*f);}
 }
};
ShellCamera::ShellCamera(const ConfigNode &b):impl_(std::make_unique<Impl>(b)){}
ShellCamera::~ShellCamera()=default;
const std::string &ShellCamera::name()const{return impl_->name;}
float ShellCamera::duration()const{return impl_->movement.curve.totalTime;}
bool ShellCamera::looping()const{return impl_->movement.curve.loop;}
bool ShellCamera::has_focus()const{return bool(impl_->focus);}
ShellCameraSample ShellCamera::sample(float seconds){
 if(!std::isfinite(seconds)||seconds<0)throw std::invalid_argument("Camera time must be finite and nonnegative");
 auto &s=*impl_;ShellCameraSample result;result.time=seconds;Vector position,direction,focus;
 // Match BookmarkObj::Step's shared time and optional focus semantics exactly.
 if(!s.movement.curve.Step(result.time,position,&direction))return result;
 if(s.focus){if(!s.focus->curve.Step(result.time,focus))return result;direction=focus-position;}
 result.position={position.x,position.y,position.z};
 double length=std::sqrt(double(direction.x)*direction.x+double(direction.y)*direction.y+double(direction.z)*direction.z);
 if(!std::isfinite(length))throw std::runtime_error("Invalid sampled camera direction");
 result.direction=length>1e-4?std::array<float,3>{float(direction.x/length),float(direction.y/length),float(direction.z/length)}:std::array<float,3>{0,0,1};
 for(float p:result.position)if(!std::isfinite(p))throw std::runtime_error("Invalid sampled camera position");
 result.active=true;return result;
}
}
