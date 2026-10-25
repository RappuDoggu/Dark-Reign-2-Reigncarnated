#pragma once
#include "reign/legacy_types.hpp"
namespace Utils {
U32 Strlen(const CH *);
CH *Strmcpy(CH *,const CH *,U32);
CH *Strdup(const CH *);
int Ansi2Unicode(CH *,U32,const char *);
char *StrFmtHex(char *,U32,U32,U32);
}
namespace reign {
enum class AnsiEncoding { ascii, windows1252 };
// Scope the installation's encoding explicitly. Unselected high bytes fail;
// Android's locale is never silently substituted for Windows GetACP().
class AnsiBinding {
 AnsiEncoding previous_;
public:
 explicit AnsiBinding(AnsiEncoding);~AnsiBinding();
 AnsiBinding(const AnsiBinding&)=delete;AnsiBinding &operator=(const AnsiBinding&)=delete;
};
}
namespace Debug::Memory::Aligning {
void *AligningAlloc(U32,U32);
void AligningFree(void *);
}
