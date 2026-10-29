#pragma once
#include "reign/config.hpp"
#include "reign/god_mesh.hpp"
namespace reign {
struct ShellLightSample {std::array<float,3> ambient{},sun{},fog{},direction{},celestialDirection{};bool sunUp=false;};
class ShellLighting {
 struct Key {float time;std::array<float,3> ambient,sun,fog;};std::vector<Key> keys_;
 float cycle_=600,start_=.3f,rise_=.2f,set_=.8f,season_=0;
public:
 explicit ShellLighting(const std::vector<ConfigNode>&);
 ShellLightSample sample(float seconds)const;
 static std::uint32_t shade(const ShellLightSample&,std::array<float,3> normal,const GodMaterial*,std::uint32_t vertex=0xffffffff);
};
}
