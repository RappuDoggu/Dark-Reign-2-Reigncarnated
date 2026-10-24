#pragma once
#include "reign/legacy_gles.hpp"
#include <string>
namespace reign {
// Runs real GPU pixel comparisons in the current framebuffer (minimum 32x32).
// Returns a human-readable report; throws on a failed comparison/GL error.
std::string renderer_checks(LegacyGles &);
}
