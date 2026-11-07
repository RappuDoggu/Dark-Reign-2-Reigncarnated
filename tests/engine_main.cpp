#include <cstdio>
int engine_checks();
int main() {
 const int failures = engine_checks();
 std::printf("Original CRC/RNG/PACK checks: %s (%d failures)\n", failures ? "FAIL" : "PASS", failures);
 return failures ? 1 : 0;
}
