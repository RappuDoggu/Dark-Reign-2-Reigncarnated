#include "reign/shell_terrain.hpp"
#include "reign/resources.hpp"
#include "reign/bitmap_manager.hpp"
#include "reign/engine_file.hpp"
#include <iostream>
#include <algorithm>
int main(int argc,char **argv){try{
 if(argc!=2)throw std::runtime_error("Usage: shell_terrain_probe full-installation-or-imported-data-root");
 reign::ResourceStore sources(argv[1]);
 for(const char *stream:{"mission","world","side"})sources.add_stream("gamedata",stream);
 if(!sources.add_archive("gamedata","packs/base.zwp")&&!sources.add_archive("gamedata","base.zwp"))throw std::runtime_error("Missing base archive");
 if(!sources.add_archive("mission","missions/system/shell.zwp")&&!sources.add_archive("mission","shell.zwp"))throw std::runtime_error("Missing shell archive");
 if(!sources.add_archive("world","worlds/sprawl.zwp"))throw std::runtime_error("Missing Sprawl world archive; import a full installation");
 sources.add_archive("side","sides/client/jda.zwp");sources.add_archive("side","sides/client/sprawler.zwp");
 auto loader=[&](const std::string &n){return sources.load("gamedata",n);};
 unsigned width=0,height=0;auto game=reign::parse_config(loader,"game.cfg");for(auto &node:game)if(node.name=="WorldInfo")for(auto &c:node.children){if(c.name=="CellMapX")width=std::get<std::int32_t>(c.arguments.at(0));if(c.name=="CellMapZ")height=std::get<std::int32_t>(c.arguments.at(0));}
 auto terrain=reign::load_shell_terrain(loader,width,height);
 auto heights=std::minmax_element(terrain.cells.begin(),terrain.cells.end(),[](auto&a,auto&b){return a.height<b.height;});
 std::cout<<"Original shell terrain: "<<width<<'x'<<height<<" pitch="<<terrain.pitch<<" cells="<<terrain.cells.size()<<" normals="<<terrain.normals.size()<<" clusters="<<terrain.clusters.size()<<" overlays="<<terrain.overlays.size()<<" height-range="<<heights.first->height<<','<<heights.second->height<<'\n';
 reign::BitmapManager bitmaps(loader,[&](const std::string &n){try{loader(n);return true;}catch(const FileSys::FileNotFound&){return false;}});
 for(auto &name:terrain.textures){auto image=bitmaps.load(name);if(!image||!image->resident)throw std::runtime_error("Terrain texture failed: "+name);std::cout<<"Texture "<<name<<' '<<image->image.width<<'x'<<image->image.height<<'\n';}
 std::cout<<"Original terrain data and bitmap decoding: PASS. World geometry/rendering is not connected.\n";
}catch(const std::exception &e){std::cerr<<"SHELL_TERRAIN_FAIL "<<e.what()<<'\n';return 1;}}
