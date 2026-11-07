#pragma once
#include <functional>
#include <string>
namespace reign {
// Android owns the surface mode. Values must come from the current native
// surface/context, not an invented DirectDraw adapter enumeration.
struct DisplaySnapshot {
 std::string renderer;
 unsigned width=0,height=0;
 bool trilinear=false,multitexture=false;
};
class DisplayOptions {
 DisplaySnapshot snapshot_;
public:
 explicit DisplayOptions(DisplaySnapshot);
 void update(DisplaySnapshot);
 void run(const std::function<void()> &);
 static void register_class();
};
}

namespace reign::display_options_detail {bool clip_guard();void mode_changed();}
