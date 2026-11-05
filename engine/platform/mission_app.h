#pragma once
#include <memory>
#include <string>
#include <cstdint>
#include <optional>
#include "vid_native.h"
namespace reign {class LegacyGles;}
namespace NativeEngine {
// One original engine instance, owned entirely by its EGL/render thread.
// The caller keeps its renderer and EGL context alive through Close/destruction.
struct MissionOptions {std::string group="missions/multiplayer",difficulty="";bool campaign=false;unsigned opponents=0,seed=1;};
struct MissionDestination {std::string group,mission;bool menu=false,campaign=false;};
class MissionApp {
 struct Impl;std::unique_ptr<Impl> impl;
public:
 MissionApp(const std::string &installation,const std::string &writable,
            const std::string &mission,reign::LegacyGles &,PresentVideo,void *,const MissionOptions &options={});
 ~MissionApp();
 MissionApp(const MissionApp &)=delete;
 MissionApp &operator=(const MissionApp &)=delete;
 std::string Diagnostics() const;
 void Control(unsigned action,float x=0,float y=0);
 void Frame();
 void SetActive(bool);
 void Close();
 bool InSimulation() const;
 bool Placing() const;
 bool CanPan() const;
 unsigned MusicBlockReasons() const;
 std::optional<MissionDestination> TakeNavigation();
 std::uint32_t SimulationCycles() const;
};
}
