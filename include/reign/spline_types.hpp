#pragma once
#include <cstddef>
// Narrow arithmetic adapter for the original standalone spline translation unit.
// No engine Vector layout is exposed across this isolated target's boundary.
namespace reign::camera_detail {
using F32=float;
struct Vector {
 float x=0,y=0,z=0;
 Vector operator-(const Vector &v)const{return {x-v.x,y-v.y,z-v.z};}
 Vector operator*(float s)const{return {x*s,y*s,z*s};}
};
}
