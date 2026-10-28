#pragma once
#include "mission_prelude.h"
#include "runcodes.h"
namespace Main {
extern RunCodes runCodes;
extern Bool active;
extern U32 thisTime,elapTime,elapLast,frameNumber;
extern F32 elapSecs;
F32 ElapTimeFrame();
F32 ElapTimeGame();
}
namespace NativeEngine {
// Engine-thread lifecycle. Milliseconds use the original wrapping U32 clock.
void InitializeFrameClock(U32 now);
void SetFrameActive(Bool active,U32 now);
bool BeginNativeFrame(U32 now);
// Uses the real native monotonic clock; caller dispatches runCodes.Process()
// only when this returns true. Frame pacing belongs to the Android scheduler.
bool BeginNativeFrame();
}
