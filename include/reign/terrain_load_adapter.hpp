#pragma once
#include "reign/shell_terrain.hpp"
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace reign::terrain_detail {
using U8=std::uint8_t;using U16=std::uint16_t;using U32=std::uint32_t;using S32=std::int32_t;using F32=float;using Bool=std::int32_t;
constexpr Bool TRUE=1;constexpr U32 RS_BLEND_MODULATE=0;
struct Vector{float x,y,z;};
struct Color{U8 b=0,g=0,r=0,a=0;operator U32()const{U32 result;std::memcpy(&result,this,4);return result;}};
static_assert(sizeof(Color)==4&&sizeof(Vector)==12);
template<class T>struct Point{T x,z;};
using Cell=ShellTerrainCell;
using Cluster=ShellTerrainCluster;
class TerrainBlock {
 const std::vector<U8> &bytes;std::size_t position=0;
public:
 explicit TerrainBlock(const std::vector<U8> &b):bytes(b){}
 void OpenBlock(const char*){position=0;}
 void CloseBlock(){if(position!=bytes.size())throw std::runtime_error("Unexpected trailing terrain data");}
 void ReadFromBlock(void *out,U32 size){if(size>bytes.size()-position)throw std::runtime_error("Truncated terrain data");std::memcpy(out,bytes.data()+position,size);position+=size;}
};
struct TerrainDecoder {
 struct HeightField{U32 cellWidth=0,cellPitch=0,cellHeight=0;F32 offsetX=0,offsetZ=0;std::vector<Cell> cellList;}heightField;
 struct Overlay{struct{U32 count;}uvs;ShellTerrainOverlay data;};
 struct Overlays{U32 count=0;std::vector<Overlay> values;Overlay &operator[](U32 i){return values.at(i);}}overlays;
 U32 normCount=0,texCount=0,overlayCount=256,clusCount=0;
 Vector normList[256]{};std::vector<std::string> textures;std::vector<Cluster> clusList;
 F32 maxFogDepth=0,maxFarPlane=0;
 struct Environment{
  F32 varAnimRate=0,waveSpeed=0,waveHeight=0,waterSpeed=0,waterAlphaTopFactor=0;
  Color waterColorBottom;U32 waterAlphaBottom=0,waterAlphaMirror=0;
  struct{F32 level=0,cloudOffset=0,windSpeed=0;Bool showClouds=0;}sky;
 }environment;
 struct WaterMaterial{Color diffuse;void SetDiffuse(Color c){diffuse=c;}}material;
 WaterMaterial *waterMaterial=&material;
 S32 AddTexture(const char *name){
  // Preserve file-table identity; bitmap allocation belongs to the renderer.
  textures.emplace_back(name);texCount=U32(textures.size());return S32(texCount-1);
 }
 S32 AddOverlay(Point<S32> size,U32 style,U32 blend){
  if(size.x<=0||size.z<=0||size.x>256||size.z>256)throw std::runtime_error("Invalid terrain overlay size");
  overlays.values.push_back({{U32((size.x+1)*(size.z+1))},{size.x,size.z,style,blend}});overlays.count=U32(overlays.values.size());return S32(overlays.count-1);
 }
 Bool Load(TerrainBlock &);
};
}
