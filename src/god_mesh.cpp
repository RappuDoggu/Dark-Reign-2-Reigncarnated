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
 std::size_t size()const{return bytes.size();}
 explicit Reader(const std::vector<std::uint8_t> &b):bytes(b){}
 void need(std::size_t n)const{if(n>bytes.size()-at)throw std::runtime_error("Truncated GOD mesh at "+std::to_string(at));}
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
 std::vector<GodBucket> buckets(unsigned version){auto n=count(12,16);std::vector<GodBucket> result;while(n--){GodBucket b;b.flags=u32();b.vertexCount=u32();b.indexCount=u32();
  if(peek()==0x9709513E){u32();b.material=material(version);}
  if(peek()==(version>1?0xF644CB33u:0x7951FC0Bu)){u32();b.texture0=texture();}
  if(version>1){if(peek()==0xF285D684){u32();b.texture1=texture();}b.teamColor=u32();b.envMap=u32();b.overlay=u32();}
result.push_back(std::move(b));}return result;}
 std::vector<GodAnimKey> keys(unsigned version,unsigned max=65535){auto n=count(version>1?48:36,max);std::vector<GodAnimKey> result;result.reserve(n);while(n--)result.push_back(key(version));return result;}
 void marker(std::uint32_t expected){if(u32()!=expected)throw std::runtime_error("Invalid GOD end marker at "+std::to_string(at-4));}

};
GodMeshPrefix prefix(Reader &r,unsigned version){
 const auto start=r.at;GodMeshPrefix m;m.version=version;m.blockBytes=r.size();m.name=r.string();m.radius=r.f32();
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
 m.buckets=r.buckets(version);
 n=r.count(4);m.vertexWeights.reserve(n);while(n--){auto count=r.count(6);std::vector<GodWeight> weights;for(unsigned i=0;i<count;++i){GodWeight w;w.weight=r.f32();w.state=r.u16();if(i<2)weights.push_back(w);}m.vertexWeights.push_back(std::move(weights));}
 m.planes=r.array<4>();m.stateMatrices=r.array<16>(64);
 n=r.count(version>1?48:36,64);m.states.reserve(n);while(n--)m.states.push_back(r.key(version));
 m.prefixBytes=r.at-start;
 auto check=[](bool ok,const char *why){if(!ok)throw std::runtime_error(why);};
 for(auto &f:m.faces){check(f.bucket<m.buckets.size(),"GOD face bucket out of range");for(unsigned i=0;i<3;++i){check(f.vertices[i]<m.vertices.size(),"GOD vertex index out of range");check(f.normals[i]<m.normals.size(),"GOD normal index out of range");check(f.uvs[i]<m.uvs.size(),"GOD UV index out of range");}}
 check(m.colors.empty()||m.colors.size()==m.vertices.size(),"GOD color count mismatch");
 check(m.vertexWeights.empty()||m.vertexWeights.size()==m.vertices.size(),"GOD vertex weight count mismatch");
 for(auto &weights:m.vertexWeights)for(auto &w:weights)check(w.state<m.states.size()&&w.state<m.stateMatrices.size(),"GOD bone index out of range");
 return m;
}
void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
void node(Reader &r,GodMesh &mesh,int parent,unsigned depth){
 require(depth<128&&mesh.nodes.size()<256,"GOD hierarchy limit exceeded");
 GodNode n;n.parent=parent;n.name=r.string();n.state=r.u32();n.flags=r.u32();
 require(n.state<mesh.geometry.states.size(),"GOD node state out of range");
 r.floats<20>();n.objectMatrix=r.floats<16>();
 const auto version=mesh.geometry.version;auto local=version>=10?r.u32():1;
 require(local<=1,"Invalid GOD local geometry flag");
 if(local){
  n.vertices=r.array<3>();n.normals=r.array<3>();n.uvs=r.array<2>();
  auto count=r.count(4);while(count--)n.colors.push_back(r.u32());
  count=r.count(16,256);while(count--){GodLocalGroup g;g.state=r.u32();g.vertexCount=r.u32();g.indexCount=r.u32();g.plane=r.u32();
   // Some legacy local group IDs exceed the root state count; retain only as metadata.
   require(g.state<=65535,"GOD local group state overflow");
   require(g.vertexCount<=65535&&g.indexCount<=65535&&g.plane<=65535,"GOD local group overflow");
   if(r.peek()==0x9709513E){r.u32();g.material=r.material(version);}
   if(r.peek()==0x7951FC0B){r.u32();g.texture=r.texture();}n.groups.push_back(std::move(g));
  }
  count=r.count(2);while(count--)n.indices.push_back(r.u16());
  for(auto index:n.indices)require(index<n.vertices.size(),"GOD local vertex index out of range");
 }
 const auto index=int(mesh.nodes.size());mesh.nodes.push_back(std::move(n));
 while(r.peek()==0xF74C51EE){r.u32();node(r,mesh,index,depth+1);}
 r.marker(0xA93EB864);
 while(r.peek()==0xB8990880){r.u32();node(r,mesh,parent,depth+1);}
}
GodMesh root(Reader &r,unsigned version,unsigned depth,unsigned &roots){
 require(depth<32&&++roots<=1024,"GOD chunk nesting/count exceeded");
 const auto start=r.at;GodMesh m;m.geometry=prefix(r,version);
 if(version<2){r.array<2>();r.buckets(version);r.u32();}
 auto count=r.count(4,1024);while(count--){GodCycle c;c.name=r.string();c.type=r.u32();c.maxFrame=r.f32();c.endFrame=r.f32();c.speed=r.f32();
  if(version>=5)c.controlFrame=r.f32();
  auto distance=r.f32();c.framesPerMeter=version>1?distance:(distance==0?0:1/distance);
  require(std::isfinite(c.framesPerMeter),"Invalid GOD animation distance");
  for(unsigned i=0;i<(version>1?7u:20u);++i)c.bounds.push_back(r.f32());
  c.states=r.keys(version,64);
  auto tracks=r.count(12,64);while(tracks--){GodAnimation a;a.state=r.u32();a.maxFrame=r.f32();a.keys=r.keys(version);
   require(a.state<m.geometry.states.size(),"GOD animation state out of range");c.animations.push_back(std::move(a));
  }m.cycles.push_back(std::move(c));
 }
 m.mrmFactor=r.f32();
 if(r.peek()==0xF18F2BDE){r.u32();GodMrm lod;
  require(r.u32()==m.geometry.vertices.size(),"GOD MRM vertex count mismatch");
  require(r.u32()==m.geometry.faces.size(),"GOD MRM face count mismatch");
  require(r.u32()==m.geometry.normals.size(),"GOD MRM normal count mismatch");
  require(r.u32()==m.geometry.uvs.size(),"GOD MRM UV count mismatch");
  auto updates=r.u32();require(updates<=65535,"GOD MRM update count overflow");
  lod.maxVertices=r.u32();lod.minVertices=r.u32();lod.minMinVertices=version>=7?r.u32():lod.minVertices;
  require(lod.maxVertices<=65535&&lod.minVertices<=65535&&lod.minMinVertices<=65535,"GOD MRM vertex limit overflow");
  std::size_t seen=0,newFaces=0;
  for(std::size_t i=0;i<m.geometry.vertices.size();++i){GodMrmVertex v;v.newFaces=r.u32();v.newNormals=r.u32();v.newUvs=r.u32();auto n=r.count(16);
   require(v.newFaces<=65535&&v.newNormals<=65535&&v.newUvs<=65535,"GOD MRM additions overflow");
   newFaces+=v.newFaces;seen+=n;require(seen<=updates&&newFaces<=m.geometry.faces.size(),"GOD MRM update totals invalid");
   while(n--){GodMrmUpdate u;u.face=r.u32();u.token=r.u32();u.indices={r.u32(),r.u32()};
    require(u.face<m.geometry.faces.size()&&u.token>=1&&u.token<=9,"GOD MRM face/token out of range");
    auto limit=u.token<=3?m.geometry.vertices.size():(u.token<=6?m.geometry.normals.size():m.geometry.uvs.size());
    require(u.indices[0]<limit&&u.indices[1]<limit,"GOD MRM replacement index out of range");v.updates.push_back(u);
   }lod.vertices.push_back(std::move(v));
  }
  require(seen==updates,"GOD MRM update total mismatch");m.mrm=std::move(lod);
 }
 node(r,m,-1,0);m.chunkIndex=r.u32();count=r.count(4,256);
 while(count--)m.chunks.push_back(root(r,version,depth+1,roots));
 r.marker(0xE3BB47F1);m.serializedBytes=r.at-start;return m;
}

}
std::pair<std::vector<std::uint8_t>,unsigned> load_block(ConfigLoader loader,const std::string &filename){
 FileSys::Session files(std::move(loader));BlockFile file;file.Open(filename.c_str(),BlockFile::READ);
 for(unsigned version:{1u,2u,3u,5u,6u,7u,8u,9u,10u,11u,12u}){
  auto key="MeshRoot "+std::to_string(version==1?4:version)+".0";U32 size=0;
  if(!file.OpenBlock(key.c_str(),FALSE,&size))continue;
  auto data=file.GetBlockPtr();file.CloseBlock();
  if(size>64*1024*1024)throw std::runtime_error("GOD mesh block too large");
  std::vector<std::uint8_t> bytes(size);if(size)std::memcpy(bytes.data(),data,size);file.Close();return {std::move(bytes),version};
 }
 throw std::runtime_error("Unsupported GOD MeshRoot version");
}
GodMeshPrefix read_god_mesh_prefix(ConfigLoader loader,const std::string &filename){auto block=load_block(std::move(loader),filename);Reader r(block.first);return prefix(r,block.second);}
GodMesh read_god_mesh(ConfigLoader loader,const std::string &filename){
 auto block=load_block(std::move(loader),filename);Reader r(block.first);unsigned roots=0;
 auto mesh=root(r,block.second,0,roots);require(r.at==r.size(),"Trailing GOD root data");return mesh;
}

}
