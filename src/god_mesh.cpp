#include "reign/config_prelude.hpp"
#include "blockfile.h"
#include "reign/god_mesh.hpp"
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace reign {
namespace {
class Reader {
 const std::vector<std::uint8_t> &bytes;
public:
 std::size_t at=0;
 explicit Reader(const std::vector<std::uint8_t> &b):bytes(b){}
 void need(std::size_t n)const{if(n>bytes.size()-at)throw std::runtime_error("Truncated GOD mesh prefix at "+std::to_string(at));}
 std::uint16_t u16(){need(2);auto v=std::uint16_t(bytes[at]|(std::uint16_t(bytes[at+1])<<8));at+=2;return v;}
 std::uint32_t u32(){need(4);std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::uint32_t(bytes[at++])<<(8*i);return v;}
 std::uint32_t peek(){need(4);auto pos=at;auto value=u32();at=pos;return value;}
 float f32(){auto bits=u32();float f;std::memcpy(&f,&bits,4);if(!std::isfinite(f))throw std::runtime_error("Nonfinite GOD value");return f;}
 template<std::size_t N>std::array<float,N> floats(){std::array<float,N> v;for(float &f:v)f=f32();return v;}
 std::uint32_t count(std::size_t stride,std::uint32_t max=65535){auto n=u32();if(n>max)throw std::runtime_error("GOD count exceeds format limit");need(std::size_t(n)*stride);return n;}
 std::string string(){auto n=u16();if(!n||n>65)throw std::runtime_error("Invalid GOD name length");need(n);if(bytes[at+n-1])throw std::runtime_error("Unterminated GOD name");std::string s(reinterpret_cast<const char*>(bytes.data()+at),n-1);if(s.find('\0')!=std::string::npos)throw std::runtime_error("Embedded NUL in GOD name");at+=n;return s;}
 template<std::size_t N>std::vector<std::array<float,N>> array(std::uint32_t max=65535){auto n=count(N*4,max);std::vector<std::array<float,N>> v;v.reserve(n);while(n--)v.push_back(floats<N>());return v;}
 GodTexture texture(){GodTexture t;t.name=string();t.type=u32();t.mipCount=u32();return t;}
 GodMaterial material(unsigned version){GodMaterial m;m.name=string();m.diffuse=floats<4>();m.specular=floats<4>();m.specularPower=f32();m.emissive=floats<4>();m.ambient=floats<4>();if(version>1&&version<10){u32();u32();u32();}return m;}
 GodAnimKey key(unsigned version){GodAnimKey k;k.frame=f32();k.type=u32();k.quaternion=floats<4>();k.position=floats<3>();if(version>1)k.scale=floats<3>();return k;}
};
GodMeshPrefix decode(const std::vector<std::uint8_t> &bytes,unsigned version){
 Reader r(bytes);GodMeshPrefix m;m.version=version;m.blockBytes=bytes.size();m.name=r.string();m.radius=r.f32();
 if(version>1){m.extent=r.floats<3>();m.offset=r.floats<3>();}
 else{auto matrix=r.floats<16>();m.offset={matrix[12],matrix[13],matrix[14]};m.extent=r.floats<3>();}
 m.scale=r.f32();r.u32();r.u32(); // quickLight, shadowPlane
 if(version>1){r.u32();r.u32();} // envMap, texture timer (milliseconds)
 if(version>11){r.f32();r.f32();r.floats<3>();} // shadow radii, p2
 if(version>5){r.u32();r.u32();} // hasTread, hasControl
 if(version>8)r.u32(); // shadowType (boolean before v11)
 r.f32();if(version>7)r.f32(); // shadowRadius, treadPerMeter
 m.vertices=r.array<3>();m.normals=r.array<3>();m.uvs=r.array<2>();
 auto n=r.count(4);m.colors.reserve(n);while(n--)m.colors.push_back(r.u32());
 n=r.count(20);m.faces.reserve(n);while(n--){GodFace f;f.bucket=r.u16();for(auto &v:f.vertices)v=r.u16();for(auto &v:f.normals)v=r.u16();for(auto &v:f.uvs)v=r.u16();m.faces.push_back(f);}
 n=r.count(12,16);m.buckets.reserve(n);while(n--){GodBucket b;b.flags=r.u32();b.vertexCount=r.u32();b.indexCount=r.u32();
  if(r.peek()==0x9709513E){r.u32();b.material=r.material(version);}
  if(r.peek()==(version>1?0xF644CB33u:0x7951FC0Bu)){r.u32();b.texture0=r.texture();}
  if(version>1){if(r.peek()==0xF285D684){r.u32();b.texture1=r.texture();}b.teamColor=r.u32();b.envMap=r.u32();b.overlay=r.u32();}
  m.buckets.push_back(std::move(b));
 }
 n=r.count(4);m.vertexWeights.reserve(n);while(n--){auto count=r.count(6);std::vector<GodWeight> weights;for(unsigned i=0;i<count;++i){GodWeight w;w.weight=r.f32();w.state=r.u16();if(i<2)weights.push_back(w);}m.vertexWeights.push_back(std::move(weights));}
 m.planes=r.array<4>();m.stateMatrices=r.array<16>(64);
 n=r.count(version>1?48:36,64);m.states.reserve(n);while(n--)m.states.push_back(r.key(version));
 m.prefixBytes=r.at;
 auto check=[](bool ok,const char *why){if(!ok)throw std::runtime_error(why);};
 for(auto &f:m.faces){check(f.bucket<m.buckets.size(),"GOD face bucket out of range");for(unsigned i=0;i<3;++i){check(f.vertices[i]<m.vertices.size(),"GOD vertex index out of range");check(f.normals[i]<m.normals.size(),"GOD normal index out of range");check(f.uvs[i]<m.uvs.size(),"GOD UV index out of range");}}
 check(m.colors.empty()||m.colors.size()==m.vertices.size(),"GOD color count mismatch");
 check(m.vertexWeights.empty()||m.vertexWeights.size()==m.vertices.size(),"GOD vertex weight count mismatch");
 for(auto &weights:m.vertexWeights)for(auto &w:weights)check(w.state<m.states.size()&&w.state<m.stateMatrices.size(),"GOD bone index out of range");
 return m;
}
}
GodMeshPrefix read_god_mesh_prefix(ConfigLoader loader,const std::string &filename){
 FileSys::Session files(std::move(loader));BlockFile file;file.Open(filename.c_str(),BlockFile::READ);
 for(unsigned version:{1u,2u,3u,5u,6u,7u,8u,9u,10u,11u,12u}){
  auto key="MeshRoot "+std::to_string(version==1?4:version)+".0";U32 size=0;
  if(!file.OpenBlock(key.c_str(),FALSE,&size))continue;
  auto data=file.GetBlockPtr();file.CloseBlock();
  if(size>64*1024*1024)throw std::runtime_error("GOD mesh block too large");
  std::vector<std::uint8_t> bytes(size);if(size)std::memcpy(bytes.data(),data,size);file.Close();return decode(bytes,version);
 }
 throw std::runtime_error("Unsupported GOD MeshRoot version");
}
}
