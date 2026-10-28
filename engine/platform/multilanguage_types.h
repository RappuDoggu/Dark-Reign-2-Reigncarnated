#pragma once
#include "strbuf.h"
// Original public identifier type, separated from Windows language UI APIs.
#define MULTILANGUAGE_MAXKEY 128
typedef StrBuf<MULTILANGUAGE_MAXKEY> MultiIdent;
