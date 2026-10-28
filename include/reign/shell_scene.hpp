#pragma once
#include <memory>
#include <string>
namespace reign {
class LegacyGles;
// Original shell data, camera curves and model poses, rendered with base textures.
// Caller clears the frame, draws this scene, then overlays the original menu UI.
class ShellScene {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 ShellScene(const std::string &installation,LegacyGles &);
 ~ShellScene();
 void draw(float seconds);
 void abandon_context();
};
}
