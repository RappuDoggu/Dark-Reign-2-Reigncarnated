#include "reign/world_projection.hpp"
#include <cmath>
#include <iostream>
#include <limits>
int main(){try{unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("World projection check "+std::to_string(checks));};auto close=[](float a,float b){return std::abs(a-b)<1e-5;};
 reign::WorldProjection camera({0,0,0},{0,0,1},1.57079632679f,1,1,11);
 auto n=camera.transform({0,0,1}),f=camera.transform({0,0,11});check(close(n[2],0)&&close(n[3],1));check(close(f[2],11)&&close(f[3],11));
 auto p=camera.transform({2,1,2});check(close(p[0]/p[3],1)&&close(p[1]/p[3],-.5f));
 check(camera.transform({0,0,-1})[3]<0);check(camera.transform({0,0,0})[3]==0);
 reign::WorldProjection rotated({10,5,20},{1,0,0},1.57079632679f,.75f,1,11);
 auto center=rotated.transform({12,5,20});check(close(center[0],0)&&close(center[1],0)&&close(center[3],2));
 auto right=rotated.transform({12,5,18});check(close(right[0]/right[3],.75f));
 auto up=rotated.transform({12,6,20});check(close(up[1]/up[3],-.5f));
 for(auto d:{std::array<float,3>{0,0,0},std::array<float,3>{0,1,0},std::array<float,3>{0,0,std::numeric_limits<float>::infinity()}}){bool rejected=false;try{reign::WorldProjection bad({0,0,0},d,1,1,1,10);}catch(const std::invalid_argument&){rejected=true;}check(rejected);}
 for(float near:{0.f,10.f,std::numeric_limits<float>::quiet_NaN()}){bool rejected=false;try{reign::WorldProjection bad({0,0,0},{0,0,1},1,1,near,10);}catch(const std::invalid_argument&){rejected=true;}check(rejected);}
 std::cout<<"Original world camera basis/inverse/projection: PASS ("<<checks<<")\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
