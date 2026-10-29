#pragma once
#include <memory>
#include <string>
#include <cstdint>
#include "vid_native.h"
namespace reign {class LegacyGles;}
namespace NativeEngine {
// One original engine instance, owned entirely by its EGL/render thread.
// The caller keeps its renderer and EGL context alive through Close/destruction.
class MissionApp {
 struct Impl;std::unique_ptr<Impl> impl;
public:
 MissionApp(const std::string &installation,const std::string &writable,
            const std::string &mission,reign::LegacyGles &,PresentVideo,void *);
 ~MissionApp();
 MissionApp(const MissionApp &)=delete;
 MissionApp &operator=(const MissionApp &)=delete;
 void Frame();
 void SetActive(bool);
 void Close();
 bool InSimulation() const;
 std::uint32_t SimulationCycles() const;
};
}
