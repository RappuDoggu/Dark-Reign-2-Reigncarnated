#pragma once
#include "mission_prelude.h"
#include "dx7/windows.h"
#include "d3d.h"
#include "clock.h"
#include "stack.h"
#include "varsys.h"
#include "str.h"
namespace Debug { namespace CallStack { void Dump(); } }
#include "mono.h"
#define LOGEXTERN
// Original release builds disable monochrome debug buffers and static guard
// sentinels. Host builds instead use AddressSanitizer/UndefinedBehaviorSanitizer.
#define DEBUG_STATIC_GUARD_BLOCK
#define DEBUG_STATIC_GUARD_BLOCK_CHECK
namespace Debug { namespace Watchdog { void Create(); void Delete(); } }
// Original DirectInput scan code; touch mapping must emit the same key value.
#include "native_keycodes.h"
#define LOGDEFLOCAL(name)
#include <type_traits>
#include <cmath>
#define _isnan std::isnan
#define LOGFMTOFF
#define LOGFMTON
namespace Debug {
namespace AtExit { Bool Register(void (*function)()); }
namespace Watchdog { void Poll(); }
}
namespace Log { void SetFlush(Bool); void CheckErrors(); void Submit(const char *subject); }
