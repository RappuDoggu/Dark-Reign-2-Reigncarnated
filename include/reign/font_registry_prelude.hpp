#pragma once
#include "reign/config_prelude.hpp"
#include "reign/font_registry.hpp"
#include "ntree.h"
#include "fscope.h"
namespace reign::registry_detail {
using GameIdent=StrCrc<64>;
struct Font {
 std::string path;std::shared_ptr<NativeFont> native;
 Bool Read(const char *file=nullptr);
};
}
