#pragma once
#include "reign/config.hpp"
#include <array>
#include <cstdint>
namespace reign {
struct ShellTerrainCell {
 float height=0;
 std::uint8_t normal=0,uv=0,texture=0,flags=0;
 std::uint16_t overlay=0;
 std::uint8_t uv1=0,texture1=0;
 std::uint32_t color=0;
 enum {cellOVERLAY=0x10,cellCOMPRESSED=0x20};
};
struct ShellTerrainOverlay {std::int32_t x=0,z=0;std::uint32_t style=0,blend=0;};
struct ShellTerrainCluster {std::uint16_t status=0;float waterHeight=0;};
struct ShellTerrainEnvironment {
 float animationRate=0,skyLevel=0,waveSpeed=0,waveHeight=0,waterSpeed=0,waterAlphaTopFactor=0,cloudOffset=0,windSpeed=0;
 std::uint32_t waterColorBottom=0,waterAlphaBottom=0,waterAlphaMirror=0;
 bool showClouds=false;
};
struct ShellTerrain {
 unsigned width=0,height=0,pitch=0;
 float offsetX=0,offsetZ=0,fogDepth=0,farPlane=0;
 ShellTerrainEnvironment environment;
 std::vector<std::array<float,3>> normals;
 std::vector<std::string> textures;
 std::vector<ShellTerrainOverlay> overlays;
 std::vector<ShellTerrainCell> cells;
 std::vector<ShellTerrainCluster> clusters;
};
// Data-only original Terrain::Load boundary for v17 maps. GPU resources,
// terrain Recalc/SetupPerf and world simulation are deliberately not initialized.
ShellTerrain load_shell_terrain(ConfigLoader loader,unsigned width,unsigned height);
}
