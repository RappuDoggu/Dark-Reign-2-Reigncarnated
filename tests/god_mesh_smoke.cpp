#include "reign/god_mesh.hpp"
#include "reign/archive.hpp"
#include <cstring>
#include <iostream>
#include <limits>
namespace {
using Bytes=std::vector<std::uint8_t>;
void u16(Bytes &b,unsigned v){b.push_back(v&255);b.push_back((v>>8)&255);}
void u32(Bytes &b,std::uint32_t v){for(unsigned i=0;i<4;++i)b.push_back((v>>(8*i))&255);}
void f32(Bytes &b,float f){std::uint32_t v;std::memcpy(&v,&f,4);u32(b,v);}
void floats(Bytes &b,unsigned n){while(n--)f32(b,1);}
void str(Bytes &b,const std::string &s){u16(b,s.size()+1);b.insert(b.end(),s.begin(),s.end());b.push_back(0);}
struct Fixture {Bytes bytes;std::size_t vertexCount,vertex,face,bucket,weight,bone;};
Fixture payload(unsigned v){Fixture f;auto &b=f.bytes;str(b,"mesh");floats(b,v>1?7:20);f32(b,2);u32(b,0);u32(b,1);
 if(v>1){u32(b,0);u32(b,100);}if(v>11)floats(b,5);if(v>5){u32(b,0);u32(b,0);}if(v>8)u32(b,0);f32(b,1);if(v>7)f32(b,1);
 f.vertexCount=b.size();u32(b,3);f.vertex=b.size();floats(b,9);u32(b,1);floats(b,3);u32(b,3);floats(b,6);u32(b,3);for(int i=0;i<3;++i)u32(b,0xffaabbcc);
 u32(b,1);f.face=b.size();u16(b,0);for(int i=0;i<3;++i)u16(b,i);for(int i=0;i<3;++i)u16(b,0);for(int i=0;i<3;++i)u16(b,i);
 f.bucket=b.size();u32(b,1);u32(b,0x1234);u32(b,3);u32(b,3);u32(b,0x9709513E);str(b,"material");floats(b,17);if(v>1&&v<10){u32(b,0);u32(b,0);u32(b,0);}
 u32(b,v>1?0xF644CB33:0x7951FC0B);str(b,"texture.pic");u32(b,1);u32(b,2);
 if(v>1){u32(b,0xF285D684);str(b,"overlay.pic");u32(b,1);u32(b,0);u32(b,1);u32(b,0);u32(b,1);}
 f.weight=b.size();u32(b,3);for(int i=0;i<3;++i){u32(b,3);for(int j=0;j<3;++j){f32(b,.25f);if(i==0&&j==0)f.bone=b.size();u16(b,0);}}
 u32(b,1);floats(b,4);u32(b,1);floats(b,16);u32(b,1);f32(b,0);u32(b,7);floats(b,7);if(v>1)floats(b,3);return f;}
Bytes block(const Bytes &p,unsigned version){Bytes b;u32(b,0x42434f44);u32(b,1);u32(b,1);b.resize(44);auto key="MeshRoot "+std::to_string(version==1?4:version)+".0";u32(b,reign::name_crc(key.c_str()));u32(b,p.size());b.insert(b.end(),p.begin(),p.end());return b;}
reign::GodMeshPrefix load(const Bytes &p,unsigned version){auto bytes=block(p,version);return reign::read_god_mesh_prefix([bytes](auto &){return bytes;},"fixture.god");}
}
int main(){try{unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("GOD check "+std::to_string(checks));};
 for(unsigned v:{1u,2u,3u,5u,6u,7u,8u,9u,10u,11u,12u}){
  auto fixture=payload(v);auto &bytes=fixture.bytes;auto m=load(bytes,v);
  check(m.version==v&&m.name=="mesh"&&m.scale==2&&m.prefixBytes==bytes.size());
  check(m.faces.size()==1&&m.faces[0].vertices[2]==2&&m.vertices.size()==3&&m.normals.size()==1);
  check(m.buckets[0].material->diffuse[0]==1&&m.buckets[0].texture0->name=="texture.pic"&&m.buckets[0].texture0->mipCount==2);
  check(bool(m.buckets[0].texture1)==(v>1));check(m.vertexWeights.size()==3&&m.vertexWeights[0].size()==2&&m.vertexWeights[0][0].weight==.25f);check(m.stateMatrices.size()==1&&m.states.size()==1&&m.states[0].scale[2]==1);
  auto rejects=[&](Bytes p){bool rejected=false;try{load(p,v);}catch(const std::exception&){rejected=true;}check(rejected);};
  for(std::size_t length=0;length<bytes.size();++length)rejects(Bytes(bytes.begin(),bytes.begin()+length));
  for(auto at:{fixture.vertexCount,fixture.bucket,fixture.weight}){auto bad=bytes;for(unsigned i=0;i<4;++i)bad[at+i]=255;rejects(bad);}
  for(auto at:{fixture.face,fixture.face+2,fixture.face+8,fixture.face+14,fixture.bone}){auto bad=bytes;bad[at]=255;bad[at+1]=255;rejects(bad);}
  auto bad=bytes;bad[fixture.vertex+2]=128;bad[fixture.vertex+3]=127;rejects(bad); // +infinity
  bad=bytes;bad[0]=0;bad[1]=0;rejects(bad);bad=bytes;bad[6]='x';rejects(bad);bad=bytes;bad[3]=0;rejects(bad);
  // This API deliberately leaves the tail opaque and reports its exact boundary.
  auto tail=bytes;tail.insert(tail.end(),{1,2,3});auto withTail=load(tail,v);check(withTail.prefixBytes==bytes.size()&&withTail.blockBytes==tail.size());
 }
 bool rejected=false;try{load(payload(12).bytes,13);}catch(const std::exception&){rejected=true;}check(rejected);
 std::cout<<"GOD versions, geometry/material/weight fields, every prefix truncation and invalid references: PASS ("<<checks<<")\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
