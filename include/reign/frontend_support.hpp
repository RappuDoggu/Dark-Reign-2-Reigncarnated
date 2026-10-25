#pragma once
#include "reign/legacy_types.hpp"
namespace Utils {
U32 Strlen(const CH *);
CH *Strmcpy(CH *,const CH *,U32);
CH *Strdup(const CH *);
}
namespace Debug::Memory::Aligning {
void *AligningAlloc(U32,U32);
void AligningFree(void *);
}
