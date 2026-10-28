#include "defines.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <cstdlib>
#define ASSERT(condition) if (!(condition)) throw std::runtime_error("Original container assertion: " #condition);
#define ERR_FATAL(message) do { std::fputs("Fatal original container invariant\n", stderr); std::abort(); } while (false)
#include "packdef.h"
#include "nlist.h"
#include "ntree.h"
#include "random.h"

static_assert(sizeof(PackInfo)==56 && offsetof(PackInfo, fileOffset)==20);
static_assert(sizeof(CH)==2 && sizeof(Bool)==4 && sizeof(F32)==4);
static_assert(sizeof(void*)==8, "This target verifies the native 64-bit path");

struct Object {
  // Force the intrusive nodes away from offset zero.
  std::uint64_t marker = 0x1122334455667788ull;
  NList<Object>::Node listNode;
  NBinTree<Object>::Node treeNode;
};

int main() {
 try {
  auto check=[](bool value) { if(!value) throw std::runtime_error("Engine ABI check failed"); };
  U32 wrap=0xffffffffu; ++wrap; check(wrap==0);
  // Parse known serialized bytes using the actual original packed struct.
  const unsigned char header[56]={'N','O','R','K',1,0,0,0,2,0,0,0,
      0xff,0xff,0xff,0x7f,3,0,0,0,0x78,0x56,0x34,0x12};
  PackInfo pack{}; std::memcpy(&pack,header,sizeof pack);
  check(pack.verId==1 && pack.dirCount==2 && pack.fileCount==3 &&
        pack.fileOffset==0x12345678u && pack.dirOffset==0x7fffffffu);
  Object a,b,c;
  NList<Object> list(&Object::listNode);
  NBinTree<Object> tree(&Object::treeNode);
  list.Append(&a); list.Append(&b); list.Append(&c);
  tree.Add(0xffffffffu,&a); tree.Add(0x80000000u,&b); tree.Add(1,&c);
  check(list.GetCount()==3 && tree.GetCount()==3);
  check(tree.Find(0xffffffffu)==&a && tree.Find(0x80000000u)==&b && tree.Find(1)==&c);
  list.Unlink(&b); tree.Unlink(&b);
  check(list.GetCount()==2 && !tree.Find(0x80000000u) && b.marker==0x1122334455667788ull);
  list.Unlink(&a); list.Unlink(&c); tree.Unlink(&a); tree.Unlink(&c);
  Random::Generator rng(0x7fffffffu);
  // Fixed independently calculated states catch an accidental 64-bit seed.
  const U32 expected[] = {0xd4387f6du,0x470ea1dfu,0x50988eeau,0x9cc7beb1u,0x450272e5u};
  for(U32 state : expected) check(rng.Raw()==state);
  std::puts("Original global definitions: packed data, intrusive pointers and 32-bit RNG: PASS");
 } catch(const std::exception &e) {std::fprintf(stderr,"%s\n",e.what());return 1;}
}
