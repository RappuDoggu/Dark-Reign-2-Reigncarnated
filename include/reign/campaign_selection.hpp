#pragma once
#include "reign/config.hpp"
#include <functional>
#include <memory>
class Bitmap;
namespace reign {
struct CampaignSelectionServices {
 std::function<ConfigLoader(const std::string &group,const std::string &mission)> open_mission;
 std::function<std::string()> default_side;
 std::function<void(std::uint32_t)> request_run_code;
 void run(const std::function<void()> &)const;
 static void register_classes();
};
namespace preview_detail {
class Resources {struct Impl;std::unique_ptr<Impl> impl_;public:explicit Resources(ConfigLoader);~Resources();};
std::unique_ptr<Resources> open(const char *,const char *);
std::unique_ptr<Bitmap> load_texture(const char *);
const char *default_side();
void request_run_code(std::uint32_t);
}
}
