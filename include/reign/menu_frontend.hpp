#pragma once
#include "reign/config.hpp"
#include <filesystem>
#include <array>
#include <functional>
#include <memory>
namespace reign {
struct FrontendBatch;struct FontBatch;class NativeFont;
struct MenuFrameReport {unsigned geometry_batches=0,font_batches=0;bool login_active=false,backdrop_active=false;};
// Owns one persistent original control tree and private player profiles.
// Serialized, one session at a time. Original shell layout; no game simulation.
class MenuFrontend {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 using GeometrySink=std::function<void(const FrontendBatch &)>;
 using FontSink=std::function<void(const NativeFont &,const FontBatch &)>;
 using SoundLoader=std::function<std::function<void(float)>(const std::string &)>;
 MenuFrontend(ConfigLoader,const std::filesystem::path &private_profiles,unsigned width,unsigned height,SoundLoader={});
 ~MenuFrontend();
 MenuFrontend(const MenuFrontend&)=delete;MenuFrontend &operator=(const MenuFrontend&)=delete;
 MenuFrameReport draw(const GeometrySink &,const FontSink &,std::uint32_t elapsed_ms,std::uint32_t step_ms);
 std::array<int,2> control_center(const std::string &);
 std::string profile_name();
 bool control_active(const std::string &);
 // Host actions are drained after original event dispatch has returned.
 std::string take_action();
 float effect_volume() const;
 void set_effect_volume(float);
 void text(const std::string &ascii);
 void key(unsigned scan,bool down);
 void pointer_move(int x,int y);
 void pointer_button(bool down,int x,int y);
 void execute_config(const std::string &);
 // Original Login notifications, for deterministic integration checks and an
 // text-input adapter. Shell transition requires the release shell archive.
 void create_profile(const std::string &);
 void login_selected_profile();
 void close();
};
}
