#pragma once
#include "events_prelude.h"
#include "input.h"
namespace NativeEngine {
// Queue on the platform/UI thread; Input::ReadEvents dispatches on engine thread.
// Coordinates must already be transformed into the engine viewport.
// Configure on the engine thread; platform event producers use Queue* only.
void SetInputViewport(S32 width,S32 height);
void QueuePointer(S32 x,S32 y,S32 button,Bool pressed); // button -1: move only
void QueueKey(U8 scanCode,Bool pressed,CH character=0);
void QueueWheel(S32 delta);
}
