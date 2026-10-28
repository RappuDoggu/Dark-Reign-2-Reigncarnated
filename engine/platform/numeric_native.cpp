#include "resource_prelude.h"
#include <cmath>
#include <cfenv>
#include <limits>
// Original low-mantissa conversion constant; host and ARM64 are little-endian.
double Float2Int::magic = 65536.0 * 65536.0 * 65536.0 * 16.0 * 1.5;
namespace {
S32 CheckedInteger(double value) {
 if(!std::isfinite(value) || value < double(std::numeric_limits<S32>::min()) || value > double(std::numeric_limits<S32>::max())) {
  std::feraiseexcept(FE_INVALID);
  return std::numeric_limits<S32>::min(); // x87 indefinite result
 }
 return static_cast<S32>(value);
}
int RoundMode(U16 token) {
 switch(token) {case 0:return FE_TONEAREST;case 1:return FE_DOWNWARD;case 2:return FE_UPWARD;case 3:return FE_TOWARDZERO;}
 throw std::invalid_argument("Invalid native rounding token");
}
U16 CurrentMode() {
 switch(std::fegetround()) {case FE_TONEAREST:return 0;case FE_DOWNWARD:return 1;case FE_UPWARD:return 2;case FE_TOWARDZERO:return 3;}
 throw std::runtime_error("Unknown floating-point rounding mode");
}
U16 SetMode(U16 token) {
 const auto previous=CurrentMode();
 if(std::fesetround(RoundMode(token))) throw std::runtime_error("Cannot set floating-point rounding mode");
 return previous;
}
}
namespace Utils {
namespace FP {
// Returned U16 values are opaque restore tokens, not x87 control words.
U16 FASTCALL SetTruncMode() { return SetMode(3); }
U16 FASTCALL SetRoundUpModeProc() { return SetMode(2); }
U16 FASTCALL SetRoundDownModeProc() { return SetMode(1); }
U16 FASTCALL SetNearestModeProc() { return SetMode(0); }
void FASTCALL RestoreModeProc(U16 token) { SetMode(token); }
}
S32 STDCALL FastFtoLProc(F32 value) { return CheckedInteger(std::nearbyint(double(value))); }
S32 STDCALL FtoL(F32 value) { return CheckedInteger(std::trunc(double(value))); }
S32 STDCALL FtoLUpProc(F32 value) { return CheckedInteger(std::ceil(double(value))); }
S32 STDCALL FtoLDownProc(F32 value) { return CheckedInteger(std::floor(double(value))); }
S32 STDCALL FtoLNearestProc(F32 value) {
 const auto previous=FP::SetNearestModeProc();
 const S32 result=FastFtoLProc(value);
 FP::RestoreModeProc(previous);
 return result;
}
}
