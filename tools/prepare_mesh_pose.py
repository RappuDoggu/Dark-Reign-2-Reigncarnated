#!/usr/bin/env python3
"""Stage reviewed original scalar animation/matrix/skinning arithmetic externally."""
import argparse,hashlib,json
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
repo=Path(__file__).resolve().parents[1];out=a.output.resolve()
if out==repo or repo in out.parents:raise ValueError('External staging required')
texts={}
for name,digest in json.loads((repo/'cmake/mesh-pose-source-hashes.json').read_text()).items():
 raw=(a.upstream/name).read_bytes()
 if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed pose source: '+name)
 texts[name]=raw.decode().replace('\r\n','\n')
def function(text,signature):
 if text.count(signature)!=1:raise ValueError('Ambiguous pose signature '+signature)
 start=text.index(signature);opening=text.index('{',start);depth=1;i=opening+1
 while depth:
  if text[i]=='{':depth+=1
  elif text[i]=='}':depth-=1
  i+=1
 return text[start:i]
header='''#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>
#define ASSERT(x) do {if(!(x))throw std::runtime_error("Original pose assertion: " #x);}while(false)
#define LOG_WARN(x) ((void)0)
namespace reign::pose_detail {
using F32=float;using U32=std::uint32_t;using Bool=bool;constexpr Bool TRUE=true;
namespace Utils{inline void Memset(void *p,int value,std::size_t size){std::memset(p,value,size);}}
struct Vector {
 F32 x=0,y=0,z=0;
 F32 Dot(const Vector &b)const{return x*b.x+y*b.y+z*b.z;}
 Vector Cross(const Vector &b)const{return {y*b.z-z*b.y,z*b.x-x*b.z,x*b.y-y*b.x};}
 Vector()=default;Vector(F32 a,F32 b,F32 c):x(a),y(b),z(c){}
 Vector operator+(const Vector &b)const{return {x+b.x,y+b.y,z+b.z};}
 Vector operator-(const Vector &b)const{return {x-b.x,y-b.y,z-b.z};}
 Vector operator*(F32 f)const{return {x*f,y*f,z*f};}
 void operator*=(F32 f){x*=f;y*=f;z*=f;}
 void operator+=(const Vector &b){x+=b.x;y+=b.y;z+=b.z;}
};
struct Matrix;
struct Quaternion {
 F32 s=1;Vector v;
 Quaternion operator+(const Quaternion &b)const{return {s+b.s,v+b.v};}
 Quaternion operator-(const Quaternion &b)const{return {s-b.s,v-b.v};}
 Quaternion operator*(F32 f)const{return {s*f,v*f};}
 F32 Dot(const Quaternion &q)const{return s*q.s+v.x*q.v.x+v.y*q.v.y+v.z*q.v.z;}
 void Set(const Matrix&);void Set(F32,const Vector&);void operator*=(const Quaternion&);
};
struct Matrix {
 Vector right;F32 rightw;Vector up;F32 upw;Vector front;F32 frontw;Vector posit;F32 positw;
 Matrix(){ClearData();}
 void ClearData();void Set(const Vector&);void Set(const Quaternion&);void SetScale(const Vector&);
 F32 Trace()const;Vector Vector_Transform(const Vector&)const;
 F32 Value(U32 r,U32 c)const {F32 v[16];std::memcpy(v,this,64);return v[r*4+c];}
 void Transform(Vector &dst,const Vector &src)const{dst=Vector_Transform(src);}
};
Matrix operator*(const Matrix&,const Matrix&);
static_assert(sizeof(Vector)==12&&sizeof(Quaternion)==16&&sizeof(Matrix)==64);
enum {animPOSITION=1,animQUATERNION=2,animSCALE=4,animDIRTY=0x80};
struct AnimKey {
 float frame=0;U32 type=0;Quaternion quaternion;Vector position,scale{1,1,1};
 void Set(const Quaternion &q){quaternion=q;type|=animDIRTY;}
 void Set(const Vector &v){position=v;type|=animDIRTY;}
 void SetScale(const Vector &v){scale=v;type|=animDIRTY|animSCALE;}
};
template<class T> struct Array {std::vector<T> values;U32 count=0;T &operator[](U32 i)const{return const_cast<T&>(values.at(i));}};
struct Animation {Array<AnimKey> keys;void SetFrame(F32,AnimKey&)const;static void Set(F32,const AnimKey&,const AnimKey&,AnimKey&);};
inline thread_local Bool statQuatError=false;
struct VertIndex {U32 count=0;std::uint16_t index[2]{};F32 weight[2]{};};
struct MeshRoot {
'''
header+=function(texts['graphics/mesh.h'],'void SetVert( Vector & dst, const Vector & src, const Matrix * tranys, const VertIndex & vindex, Bool doMultiWeight) const')+'\n};\n}\n'
source='#include "mesh_pose_slice.hpp"\nnamespace reign::pose_detail {\n'
for sig in ['void Quaternion::operator*=(const Quaternion &qq)','void Quaternion::Set(F32 rads, const Vector &vv)','void Matrix::ClearData()','void Matrix::Set( const Vector &position)','void Matrix::SetScale( const Vector &scale)','void Matrix::Set( const Quaternion &qq)','F32 Matrix::Trace() const','void Quaternion::Set( const Matrix &matrix)','\nVector Matrix::Vector_Transform (const Vector &v) const','\nMatrix operator* (const Matrix &A, const Matrix &B)']:
 source+=function(texts['util/mathtypes.cpp'],sig)+'\n'
for sig in ['void Animation::SetFrame( F32 frame, AnimKey &state) const','void Animation::Set( F32 frame, const AnimKey &lastKey, const AnimKey &thisKey, AnimKey &state)']:
 source+=function(texts['graphics/anim.cpp'],sig)+'\n'
source+='}\n'
out.mkdir(parents=True,exist_ok=True)
for name,text in [('mesh_pose_slice.hpp',header),('mesh_pose_slice.cpp',source)]:
 path=out/name
 if not path.exists() or path.read_text()!=text:path.write_text(text)
