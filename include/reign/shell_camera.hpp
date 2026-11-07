#pragma once
#include "reign/config.hpp"
#include <array>
#include <memory>
#include <string>
namespace reign {
struct ShellCameraSample {
 std::array<float,3> position{},direction{};
 float time=0;
 bool active=false;
};
// Owns original BookmarkObj::Curve lists. No GameObj or world renderer is implied.
class ShellCamera {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 explicit ShellCamera(const ConfigNode &bookmark);
 ~ShellCamera();
 ShellCamera(const ShellCamera&)=delete;
 ShellCamera &operator=(const ShellCamera&)=delete;
 const std::string &name() const;
 float duration() const;
 bool looping() const;
 bool has_focus() const;
 ShellCameraSample sample(float seconds);
};
// Plays imported bookmarks and holds the last valid pose at completion.
class ShellCameraTimeline {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 explicit ShellCameraTimeline(const std::vector<ConfigNode> &objects);
 ~ShellCameraTimeline();
 void select(const std::string &name,float now);
 ShellCameraSample sample(float now);
 bool finished(float now) const;
 std::string name(float now) const;
};
}
