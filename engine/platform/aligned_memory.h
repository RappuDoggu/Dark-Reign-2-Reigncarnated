#pragma once
#include <cstdlib>
#include <cstddef>
#include <new>
#include <stdexcept>
namespace Debug::Memory {
class Aligning {
public:
static inline void *AligningAlloc(U32 size, U32 alignment) {
  if (!size) return nullptr;
  std::size_t a = alignment ? alignment : alignof(std::max_align_t);
  if ((a & (a - 1)) != 0) throw std::invalid_argument("Non-power-of-two engine alignment");
  if (a < sizeof(void *)) a = sizeof(void *);
  void *p=nullptr;
  if (posix_memalign(&p,a,size)) throw std::bad_alloc();
  return p;
}
static inline void AligningFree(void *p) { std::free(p); }
};
}
