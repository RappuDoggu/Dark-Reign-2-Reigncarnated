#pragma once
#include <memory>
#include <string>
#include <vector>
namespace reign {
class LegacyGles;
struct ShellEffectStats{unsigned emitters=0,particles=0,waterClusters=0,groundSplats=0,waterSplats=0;};
// Original shell data, camera curves and model poses, rendered with base textures.
// Caller clears the frame, draws this scene, then overlays the original menu UI.
class ShellScene {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 ShellScene(const std::string &installation,LegacyGles &);
 ~ShellScene();
 void draw(float seconds);
 ShellEffectStats effect_stats()const;
 void abandon_context();
 std::string ambient_sound()const;
 std::vector<std::string> take_sounds();
 void finish_frame(float seconds);
};
}
