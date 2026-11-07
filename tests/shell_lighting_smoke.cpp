#include "reign/shell_lighting.hpp"
#include "reign/god_pose.hpp"
#include <cmath>
#include <iostream>
#include <limits>
int main(){unsigned checks=0;auto check=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Lighting check "+std::to_string(checks));};
 std::string config=R"(Environment(){Light(){CycleTime(100);StartTime(0);SunriseStartTime(0.2);SunsetEndTime(0.8);Colors(){Add(){Fraction(0);Ambient(89,134,67,255);Sun(0,186,37,255);Fog(0,0,0,255);}}}})";
 auto parse=[&]{return reign::parse_config([&](const std::string&){return std::vector<std::uint8_t>(config.begin(),config.end());},"game.cfg");};
 reign::ShellLighting light(parse());auto sample=light.sample(0);check(std::abs(sample.ambient[0]-89/255.f)<1e-6);check(std::abs(sample.ambient[1]-134/255.f)<1e-6);check(std::abs(sample.ambient[2]-67/255.f)<1e-6);
 check(!sample.sunUp);check(light.sample(50).sunUp);check(std::abs(sample.celestialDirection[0]*sample.celestialDirection[0]+sample.celestialDirection[1]*sample.celestialDirection[1]+sample.celestialDirection[2]*sample.celestialDirection[2]-1)<1e-5);
 auto direction=sample.direction;check(std::abs(direction[0]*direction[0]+direction[1]*direction[1]+direction[2]*direction[2]-1)<1e-5);
 check(reign::ShellLighting::shade(sample,direction,nullptr)==0xff59ff68);
 for(auto &x:direction)x=-x;check(reign::ShellLighting::shade(sample,direction,nullptr)==0xff598643);
 check(light.sample(100).direction==sample.direction);check(light.sample(25).direction!=sample.direction);
 bool failed=false;try{light.sample(std::numeric_limits<float>::quiet_NaN());}catch(...){failed=true;}check(failed);
 config=R"(Environment(){Light(){CycleTime(100);StartTime(0);Colors(){Add(){Fraction(0.25);Ambient(0,0,0,255);Sun(0,0,0,255);Fog(0,0,0,255);}Add(){Fraction(0.75);Ambient(200,100,50,255);Sun(0,0,0,255);Fog(0,0,0,255);}}}})";
 reign::ShellLighting wrap(parse());auto mid=wrap.sample(0);check(std::abs(mid.ambient[0]-100/255.f)<1e-6);check(wrap.sample(25).ambient[0]==0);check(std::abs(wrap.sample(50).ambient[1]-50/255.f)<1e-6);
 config=R"(Environment(){Light(){CycleTime(0);Colors(){Add(){Fraction(0);Ambient(1,2,3);Sun(1,2,3);Fog(1,2,3);}}}})";failed=false;try{reign::ShellLighting bad(parse());}catch(...){failed=true;}check(failed);
 std::cout<<"Shell lighting, sun direction, cycle wrapping, original colors and invalid input: PASS ("<<checks<<")\n";
}
