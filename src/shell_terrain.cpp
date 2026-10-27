#include "reign/config_prelude.hpp"
#include "blockfile.h"
#include "reign/terrain_load_adapter.hpp"
namespace reign {
ShellTerrain load_shell_terrain(ConfigLoader loader,unsigned width,unsigned height){
 if(width<4||height<4||width>2048||height>2048||width%4||height%4)throw std::invalid_argument("Terrain dimensions must be bounded multiples of four");
 FileSys::Session files(std::move(loader));BlockFile file;file.Open("terrain.blk",BlockFile::READ);
 U32 bytes=file.SizeOfBlock(Crc::CalcStr("terraindata"));if(bytes>128*1024*1024)throw std::runtime_error("Terrain block too large");
 std::vector<U8> block(bytes);file.ReadBlock("terraindata",block.data(),bytes);file.Close();
 terrain_detail::TerrainDecoder decoder;auto &field=decoder.heightField;
 // Matches HeightField::Setup: pitch width+1; height is stored without a border row.
 field.cellWidth=width;field.cellPitch=width+1;field.cellHeight=height;
 field.cellList.resize(std::size_t(width+1)*height);
 decoder.clusCount=(width/4)*(height/4);decoder.clusList.resize(decoder.clusCount);
 terrain_detail::TerrainBlock input(block);decoder.Load(input);
 ShellTerrain result;result.width=width;result.height=height;result.pitch=width+1;result.offsetX=field.offsetX;result.offsetZ=field.offsetZ;result.fogDepth=decoder.maxFogDepth;result.farPlane=decoder.maxFarPlane;
 auto finite=[](float f){if(!std::isfinite(f))throw std::runtime_error("Nonfinite terrain value");};
 auto &e=decoder.environment;result.environment={e.varAnimRate,e.sky.level,e.waveSpeed,e.waveHeight,e.waterSpeed,e.waterAlphaTopFactor,e.sky.cloudOffset,e.sky.windSpeed,e.waterColorBottom,e.waterAlphaBottom,e.waterAlphaMirror,e.sky.showClouds!=0};
 for(float f:{e.varAnimRate,e.sky.level,e.waveSpeed,e.waveHeight,e.waterSpeed,e.waterAlphaTopFactor,e.sky.cloudOffset,e.sky.windSpeed})finite(f);
 if(e.sky.showClouds!=0&&e.sky.showClouds!=1)throw std::runtime_error("Invalid terrain cloud flag");
 finite(result.offsetX);finite(result.offsetZ);finite(result.fogDepth);finite(result.farPlane);
 for(U32 i=0;i<decoder.normCount;++i){auto &v=decoder.normList[i];finite(v.x);finite(v.y);finite(v.z);result.normals.push_back({v.x,v.y,v.z});}
 for(auto &c:field.cellList)finite(c.height);for(auto &c:decoder.clusList)finite(c.waterHeight);
 for(auto &o:decoder.overlays.values)result.overlays.push_back(o.data);
 result.textures=std::move(decoder.textures);result.cells=std::move(field.cellList);result.clusters=std::move(decoder.clusList);return result;
}
}
