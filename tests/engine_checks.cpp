#include "reign/legacy_types.hpp"
#include "crc.h"
#include "random.h"
constexpr U32 U8_MAX = 255;
#include "packdef.h"
#include <array>
#include <cstring>
#include <cmath>
static_assert(sizeof(PackInfo) == 56, "Original on-disk PACK header must stay 56 bytes");
// Bit-at-a-time oracle, independent of the engine's lookup table.
static U32 reference(const U8 *data, U32 count, U32 seed = 0) {
 U32 r = ~seed;
 for (U32 i = 0; i < count; ++i) {
  r ^= U32(data[i]) << 24;
  for (int bit = 0; bit < 8; ++bit)
   r = (r << 1) ^ ((r & 0x80000000u) ? 0x04c11db7u : 0u);
 }
 return ~r;
}
int engine_checks() {
 int failed = 0;
 auto check = [&](bool ok) { if (!ok) ++failed; };
 const auto *digits = reinterpret_cast<const U8 *>("123456789");
 check(Crc::Calc(digits, 9) == 0xfc891918u);
 check(Crc::Calc(nullptr, 0, 0x12345678u) == 0x12345678u);
 std::array<U8, 256> all{};
 for (U32 i = 0; i < all.size(); ++i) all[i] = U8(i);
 check(Crc::Calc(all.data(), all.size()) == reference(all.data(), all.size()));
 for (U32 n = 0; n <= all.size(); ++n) {
  auto first = Crc::Calc(all.data(), n, 0xa5a55a5au);
  check(Crc::Calc(all.data() + n, all.size() - n, first) == reference(all.data(), all.size(), 0xa5a55a5au));
 }
 check(Crc::CalcStr("Intro") == 0xa521e37bu); // run-code ID embedded in GameGod::Init
 check(Crc::CalcStr("MiSsIoN") == Crc::CalcStr("mission"));
 check(Crc::CalcLower("MiSsIoN", 7) == Crc::CalcStr("mission"));
 const CH wide[] = {0x0041, 0x005a, 0};
 const U8 wide_bytes[] = {0, 'a', 0, 'z'};
 check(Crc::CalcStr(wide) == reference(wide_bytes, 4));
 std::array<char, 256> extended{};
 for (int i = 1; i < 256; ++i) extended[i-1] = static_cast<char>(i);
 check(Crc::CalcStr(extended.data()) == Crc::CalcLower(extended.data(), 255));
 Random::Generator rng(0);
 // Golden states calculated independently with 32-bit wrap then a 16-bit rotation.
 constexpr U32 expected[] = {2424832u, 2447681u, 2354204579u, 453781130u, 3689353068u};
 for (auto value : expected) check(rng.Raw() == value);
 rng.SetSeed(1234);
 Random::Generator same(1234);
 for (int i = 0; i < 1000; ++i) check(rng.Raw() == same.Raw());
 rng.SetSeed(0); same.SetSeed(0);
 check(rng.Integer(0) == 0); same.Raw();
 check(rng.Raw() == same.Raw()); // Integer(0) still advances the stream
 for (int i = 0; i < 1000; ++i) check(rng.Integer(37) < 37);
 for (int i = 0; i < 1000; ++i) { auto v = rng.Float(); check(std::isfinite(v) && v >= 0 && v <= 1); }
 check(_rotr(0x12345678u, 0) == 0x12345678u);
 check(_rotr(0x12345678u, 32) == 0x12345678u);
 return failed;
}
