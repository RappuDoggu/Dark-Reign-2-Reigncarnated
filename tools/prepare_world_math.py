#!/usr/bin/env python3
"""Stage original scalar camera matrix arithmetic, excluding x86 assembly paths."""
import argparse,hashlib,json
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
repo=Path(__file__).resolve().parents[1];out=a.output.resolve()
if out==repo or repo in out.parents:raise ValueError('External staging required')
texts={}
for name,digest in json.loads((repo/'cmake/world-math-source-hashes.json').read_text()).items():
 raw=(a.upstream/name).read_bytes()
 if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed matrix source: '+name)
 texts[name]=raw.decode().replace('\r\n','\n')
def function(text,signature):
 if text.count(signature)!=1:raise ValueError('Unreviewed function signature '+signature)
 start=text.index(signature);opening=text.index('{',start);depth=1;i=opening+1
 while depth:
  if text[i]=='{':depth+=1
  elif text[i]=='}':depth-=1
  i+=1
 return text[start:i]
h=texts['util/mathtypes.h'];cpp=texts['util/mathtypes.cpp']
header='''#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
namespace reign::world_detail {
using F32=float;using S32=std::int32_t;
constexpr F32 F32_EPSILON=1.192092896e-07F;
namespace Utils {inline void Memset(void *p,int c,std::size_t n){std::memset(p,c,n);}}
struct Vector {
 F32 x=0,y=0,z=0;
 Vector()=default;Vector(F32 a,F32 b,F32 c):x(a),y(b),z(c){}
 F32 Normalize();
'''
for signature in ['friend inline Vector Cross (const Vector &A,  const Vector &B)','F32 Dot(const Vector &vv) const','inline F32 Magnitude2() const','inline F32 Magnitude() const']:
 header+=function(h,signature)+'\n'
header+='''};
struct Matrix {
 Vector right;F32 rightw;Vector up;F32 upw;Vector front;F32 frontw;Vector posit;F32 positw;
 Matrix(){ClearData();}
 void ClearData();void Zero();void SetProjection(F32,F32,F32,F32);
 void SetInverse(const Matrix&);void SetFromFront(const Vector&);
 void Transform_Vectors(const F32*,const F32*,S32,S32)const;
};
static_assert(sizeof(Matrix)==64 && sizeof(Vector)==12);
}
'''
source='#include "world_math_slice.hpp"\nnamespace reign::world_detail {\n'
for signature in ['F32 Vector::Normalize()','void Matrix::ClearData()','void Matrix::Zero()','void Matrix::SetProjection(F32 nearPlane, F32 farPlane, F32 fov, F32 aspectHW)','void Matrix::SetInverse( const Matrix &m)','void Matrix::SetFromFront(const Vector & _front)','\nvoid Matrix::Transform_Vectors (']:
 source+=function(cpp,signature)+'\n'
source+='}\n'
out.mkdir(parents=True,exist_ok=True)
for name,text in [('world_math_slice.hpp',header),('world_math_slice.cpp',source)]:
 path=out/name
 if not path.exists() or path.read_text()!=text:path.write_text(text)

# Tables used only by the terrain GPU integration probe, not a replacement renderer.
def array(text,signature):
 start=text.index(signature);end=text.index('};',start)+2
 return text[start:end]
tables='#pragma once\n#include <cstdint>\nnamespace reign::world_detail {\nusing U16=std::uint16_t;constexpr float MAXTEXCOORD=1.0f;\nstruct UVPair{float u,v;constexpr UVPair(float x,float y):u(x),v(y){}};\n'
tables+=array(texts['graphics/terrain.cpp'],'UVPair cellUVList[4 * 4 * 3]').replace('UVPair cellUVList','inline const UVPair cellUVList')+'\n'
tables+=array(texts['graphics/terrain_render_vertextl.cpp'],'U16 clusterI[96]').replace('U16 clusterI','inline const U16 clusterI')+'\n}\n'
path=out/'terrain_probe_tables.hpp'
if not path.exists() or path.read_text()!=tables:path.write_text(tables)
