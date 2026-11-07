#include "spline.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
using reign::camera_detail::Vector;
using reign::camera_detail::CubicSpline;
int main(){try{
 unsigned checks=0;auto check=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Spline check "+std::to_string(checks));};
 auto near=[](Vector a,Vector b){return std::fabs(a.x-b.x)<.0005f&&std::fabs(a.y-b.y)<.0005f&&std::fabs(a.z-b.z)<.0005f;};
 Vector start{341.759003f,70.235558f,318.126526f},end{230.048218f,27.157103f,288.156342f};
 Vector a{43.4924f,0,-90.0467f},b{0,0,100};CubicSpline spline;spline.SetupHermite(start,a,end,b,3);
 Vector tangent;check(near(spline.Step(0,&tangent),start));check(near(tangent,a));check(near(spline.Step(3,&tangent),end));check(near(tangent,b));
 for(float seconds:{.5f,1.f,1.5f,2.f,2.5f}){
  float t=seconds/3, t2=t*t,t3=t2*t;
  auto coord=[&](float p,float v,float q,float w){return (2*t3-3*t2+1)*p+(t3-2*t2+t)*v+(-2*t3+3*t2)*q+(t3-t2)*w;};
  check(near(spline.Step(seconds),{coord(start.x,a.x,end.x,b.x),coord(start.y,a.y,end.y,b.y),coord(start.z,a.z,end.z,b.z)}));
 }
 CubicSpline slow;slow.SetupHermite(start,a,end,b,6);check(near(slow.Step(3),spline.Step(1.5f)));
 std::cout<<"Original Hermite spline endpoints, tangents, interior oracle and duration: PASS ("<<checks<<" checks)\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
