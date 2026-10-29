#include "reign/shell_camera.hpp"
#include <cmath>
#include <iostream>
#include <limits>
int main(){try{
 unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("Camera check "+std::to_string(checks));};
 auto parse=[](std::string text){return reign::parse_config([text](const std::string &){return std::vector<std::uint8_t>(text.begin(),text.end());},"fixture.cfg");};
 const std::string track=R"(Loop(0);Segment(){Matrix(){Pos(0,0,0);Front(1,0,0);}Strength(2);Time(2);}Segment(){Matrix(){Pos(2,0,0);Front(1,0,0);}Strength(2);Time(7);})";
 auto nodes=parse("BookmarkObj(\"test\"){Curve(){Movement(){"+track+"}}}");
 reign::ShellCamera camera(nodes[0]);check(camera.name()=="test"&&camera.duration()==2&&!camera.looping()&&!camera.has_focus());
 for(float t:{0.f,.5f,1.f,1.5f}){auto s=camera.sample(t);check(s.active&&std::abs(s.position[0]-t)<1e-5&&s.direction[0]==1&&s.time==t);}
 auto end=camera.sample(2);check(!end.active&&end.time==2);check(!camera.sample(100).active);
 for(float t:{-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){bool rejected=false;try{camera.sample(t);}catch(const std::invalid_argument&){rejected=true;}check(rejected);}
 auto looping=track;looping.replace(5,1,"1");nodes=parse("BookmarkObj(\"loop\"){Curve(){Movement(){"+looping+"}}}");reign::ShellCamera loop(nodes[0]);check(loop.looping()&&loop.duration()==9);
 auto a=loop.sample(.5f),b=loop.sample(18.5f);check(b.active&&b.time==.5f&&a.position==b.position);check(loop.sample(9).position==loop.sample(0).position);
 const std::string one=R"(Loop(1);Segment(){Matrix(){Pos(4,5,6);Front(0,0,1);}Strength(0);Time(3);})";
 nodes=parse("BookmarkObj(\"single\"){Curve(){Movement(){"+one+"}}}");reign::ShellCamera single(nodes[0]);check(single.duration()==3&&single.looping());check(single.sample(7).active&&single.sample(7).position==std::array<float,3>{4,5,6});
 auto focus=track;std::size_t p=0;while((p=focus.find(",0,0);Front",p))!=std::string::npos){focus.replace(p,11,",0,2);Front");p+=11;}
 nodes=parse("BookmarkObj(\"focus\"){Curve(){Movement(){"+track+"}Focus(){"+focus+"}}}");reign::ShellCamera aimed(nodes[0]);auto s=aimed.sample(1);check(aimed.has_focus()&&s.active&&s.direction[2]==1&&s.direction[0]==0);
 for(std::string invalid:std::vector<std::string>{"Loop(0);", "Loop(2);"+track.substr(8),"Loop(0);Segment(){Matrix(){Pos(0,0,0);Front(1,0,0);}Strength(1);Time(0);}"}){
  nodes=parse("BookmarkObj(\"invalid\"){Curve(){Movement(){"+invalid+"}}}");bool rejected=false;try{reign::ShellCamera bad(nodes[0]);}catch(const std::runtime_error&){rejected=true;}check(rejected);
 }
 std::cout<<"Original camera build/loop/end/focus/invalid-data checks: PASS ("<<checks<<")\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
