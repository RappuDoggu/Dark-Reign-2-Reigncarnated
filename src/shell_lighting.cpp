#include "reign/shell_lighting.hpp"
#include "reign/god_pose.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace reign {
namespace {
const ConfigNode *child(const ConfigNode &n,const char *name){for(auto &c:n.children)if(c.name==name)return &c;return nullptr;}
float number(const ConfigValue &v){float f=std::holds_alternative<float>(v)?std::get<float>(v):float(std::get<std::int32_t>(v));if(!std::isfinite(f))throw std::runtime_error("Nonfinite lighting setting");return f;}
float setting(const ConfigNode &n,const char *name,float value){auto p=child(n,name);return p?number(p->arguments.at(0)):value;}
std::array<float,3> color(const ConfigNode &n,const char *name){auto p=child(n,name);if(!p||p->arguments.size()<3)throw std::runtime_error("Missing light color");std::array<float,3> c;for(int i=0;i<3;i++){c[i]=number(p->arguments[i]);if(c[i]<0||c[i]>255)throw std::runtime_error("Invalid light color");}return c;}
}
ShellLighting::ShellLighting(const std::vector<ConfigNode> &config){
 const ConfigNode *light=nullptr;for(auto &n:config)if(n.name=="Environment")light=child(n,"Light");if(!light)throw std::runtime_error("Missing shell environment lighting");
 cycle_=setting(*light,"CycleTime",cycle_);start_=setting(*light,"StartTime",start_);rise_=setting(*light,"SunriseStartTime",rise_);set_=setting(*light,"SunsetEndTime",set_);season_=setting(*light,"Season",season_);
 if(cycle_<=0||start_<0||start_>1||rise_<0||set_>1||rise_>=set_)throw std::runtime_error("Invalid lighting cycle");
 auto colors=child(*light,"Colors");if(!colors)throw std::runtime_error("Missing environment colors");for(auto &n:colors->children)if(n.name=="Add"){float time=setting(n,"Fraction",-1);if(time<0||time>1)throw std::runtime_error("Invalid light key");keys_.push_back({time,color(n,"Ambient"),color(n,"Sun"),color(n,"Fog")});}
 if(keys_.empty()||keys_.size()>256)throw std::runtime_error("Invalid light color count");std::sort(keys_.begin(),keys_.end(),[](auto&a,auto&b){return a.time<b.time;});for(std::size_t i=1;i<keys_.size();i++)if(keys_[i-1].time==keys_[i].time)throw std::runtime_error("Duplicate light key");
}
ShellLightSample ShellLighting::sample(float seconds)const{
 if(!std::isfinite(seconds)||seconds<0)throw std::runtime_error("Invalid light time");float time=std::fmod(std::fmod(seconds,cycle_)/cycle_+start_,1.f);
 std::size_t left=keys_.size()-1;for(std::size_t i=0;i<keys_.size();i++)if(keys_[i].time<=time)left=i;const auto &a=keys_[left],&b=keys_[(left+1)%keys_.size()];float range=b.time-a.time;if(range<0)range+=1;float portion=time-a.time;if(portion<0)portion+=1;float f=range==0?0:portion/range;
 ShellLightSample out;for(int i=0;i<3;i++){// Original FillInColor quantizes interpolated channels to U8.
  out.ambient[i]=std::uint8_t(a.ambient[i]*(1-f)+b.ambient[i]*f)/255.f;out.sun[i]=std::uint8_t(a.sun[i]*(1-f)+b.sun[i]*f)/255.f;out.fog[i]=std::uint8_t(a.fog[i]*(1-f)+b.fog[i]*f)/255.f;
 }
 float day=set_-rise_;float azimuth=time>=rise_&&time<set_?(time-rise_)/day:(time>=set_?(time-set_):(time+1-set_))/(1-day);
 out.direction=original_sun_direction(azimuth,season_);return out;
}
std::uint32_t ShellLighting::shade(const ShellLightSample &s,std::array<float,3> n,const GodMaterial *m,std::uint32_t vertex){
 float dot=std::max(0.f,n[0]*s.direction[0]+n[1]*s.direction[1]+n[2]*s.direction[2]);std::uint32_t out=vertex&0xff000000;
 for(int i=0;i<3;i++){unsigned shift=16-i*8;float diffuse=m?m->diffuse[i]:1.f;float channel=float((vertex>>shift)&255)*diffuse*(s.ambient[i]+s.sun[i]*dot);out|=std::uint32_t(std::clamp(std::lround(channel),0l,255l))<<shift;}return out;
}
}
