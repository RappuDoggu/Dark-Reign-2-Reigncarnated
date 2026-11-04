#pragma once
#include "reign/config.hpp"
namespace reign {
// Preserve the imported original city lobby's layout, skins, icons and strings.
// Bind its offline subset explicitly; never execute absent network commands.
std::string original_instant_layout(ConfigLoader);
}
