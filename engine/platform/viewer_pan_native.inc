// Included only in the SHA-verified original viewer_standard.cpp.
// Non-virtual method: no original data members or vtable entries are changed.
namespace Viewer {
void Standard::PanNative(F32 x,F32 z)
{
  // Match the original scroll path: move the horizontal target, retaining
  // height, targetHeight and pitch. Set(worldPosition, ...) treats world Y as
  // terrain-relative height and repeatedly adds the floor during simulation.
  targetPos.x += x;
  targetPos.z += z;
  smoothPos = FALSE;
  state = STATE_FREE;
}
}
