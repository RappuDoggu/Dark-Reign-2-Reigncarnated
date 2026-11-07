#include "reign/shell_terrain.hpp"
#include "reign/archive.hpp"
#include <cstring>
#include <iostream>
#include <limits>
namespace {
using Bytes=std::vector<std::uint8_t>;
void u32(Bytes &b,std::uint32_t v){for(int i=0;i<4;++i)b.push_back(std::uint8_t(v>>(8*i)));}
void f32(Bytes &b,float f){std::uint32_t v;std::memcpy(&v,&f,4);u32(b,v);}
Bytes payload(){Bytes b;u32(b,17);f32(b,0);f32(b,0);u32(b,1);f32(b,0);f32(b,1);f32(b,0);u32(b,1);u32(b,12);for(char c:std::string("terrain.tga"))b.push_back(c);b.push_back(0);
 u32(b,1);u32(b,1);u32(b,1);u32(b,0);u32(b,0);f32(b,1);
 for(int i=0;i<20;++i){f32(b,float(i));b.insert(b.end(),{0,2,0,0x10,0,0,0,0});u32(b,0xff332211);}
 for(float f:{20.f,30.f,100.f,1.f,2.f,3.f})f32(b,f);u32(b,0x44332211);f32(b,.5f);u32(b,0x88332211);f32(b,10);f32(b,2);u32(b,1);
 b.push_back(1);b.push_back(0);f32(b,8);return b;}
Bytes block(const Bytes &p){Bytes b;u32(b,0x42434f44);u32(b,1);u32(b,1);b.resize(44);u32(b,reign::name_crc("terraindata"));u32(b,p.size());b.insert(b.end(),p.begin(),p.end());return b;}
reign::ShellTerrain load(const Bytes &p,unsigned w=4,unsigned h=4){auto bytes=block(p);return reign::load_shell_terrain([bytes](auto &){return bytes;},w,h);}
}
int main(){try{unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("Terrain check "+std::to_string(checks));};auto bytes=payload();auto t=load(bytes);
 check(t.width==4&&t.height==4&&t.pitch==5&&t.cells.size()==20);check(t.normals.size()==1&&t.normals[0][1]==1);check(t.textures==std::vector<std::string>{"terrain.tga"});check(t.cells[19].height==19&&t.cells[19].color==0xff332211&&t.cells[19].flags==0x10);check(t.overlays.size()==1&&t.overlays[0].x==1);check(t.clusters.size()==1&&t.clusters[0].status==1&&t.clusters[0].waterHeight==8);check(t.farPlane==100&&t.fogDepth==20);check(t.environment.showClouds&&t.environment.skyLevel==30&&t.environment.waveHeight==2&&t.environment.waterColorBottom==0x44332211&&t.environment.waterAlphaMirror==0x88);
 auto rejects=[&](const Bytes &p){bool rejected=false;try{load(p);}catch(const std::exception&){rejected=true;}check(rejected);};
 for(std::size_t length=0;length<bytes.size();++length)rejects(Bytes(bytes.begin(),bytes.begin()+length));
 for(std::size_t at:{std::size_t(0),std::size_t(12),std::size_t(28),std::size_t(32)}){auto bad=bytes;bad[at]=255;bad[at+1]=255;bad[at+2]=255;bad[at+3]=127;rejects(bad);}
 auto bad=bytes;bad.push_back(0);rejects(bad);
 bad=bytes;bad[36]=0;rejects(bad); // embedded NUL in texture name
 bool rejected=false;try{load(bytes,5,4);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
 std::cout<<"Original terrain v17 fields, every truncation, bounded counts/names: PASS ("<<checks<<")\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
