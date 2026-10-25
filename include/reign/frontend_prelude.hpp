#pragma once
// Compile boundary: declarations here do not provide runtime implementations.
#include "control_types.hpp"
#include "reign/frontend_font.hpp"
#include "reign/frontend_support.hpp"
struct Pix; struct VertexTL;
#include "iface.h"
using PathString=StrBuf<270>;
#define LOGEXTERN
namespace Utils {

char *Strmcpy(char *,const char *,U32);
int Ansi2Unicode(CH *,U32,const char *);
char *StrFmtHex(char *,U32,U32,U32=0);
}

#define __EVENT_H
constexpr S32 DIK_TAB=0x0F;
namespace reign { void deactivate_control_movie(Bitmap *); }
#include "control_array.hpp"
#include "iface_messagebox.h"

constexpr S32 DIK_RETURN=0x1C, DIK_SPACE=0x39, DIK_ESCAPE=0x01;

#include "iface_priv.h"
namespace IFace { SystemData &FrontendData(); }
#include "icsystembutton.h"

