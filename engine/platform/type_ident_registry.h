#pragma once
#include "utiltypes.h"
namespace NativeEngine {
// Reaper object IDs remain 32-bit. Type names use checked registry tokens,
// never pointer casts. The registry owns unresolved names until taken/reset.
U32 StoreTypeIdent(const char *name);
GameIdent TakeTypeIdent(U32 token);
void ClearTypeIdents();
std::size_t PendingTypeIdents();
}
