#include "reign/platform.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
int main() {
 int failed = 0;
 auto check = [&](bool v) { if (!v) ++failed; };
 auto rejects = [&](auto op) { try { op(); ++failed; } catch (const std::exception &) {} };
 reign::Lifecycle life;
 check(life.state() == reign::State::paused);
 life.resume(); check(life.state() == reign::State::paused);
 life.surface(true); check(life.state() == reign::State::running);
 life.pause(); check(life.state() == reign::State::paused);
 life.resume(); life.surface(false); check(life.state() == reign::State::paused);
 life.destroy(); life.resume(); life.surface(true); check(life.state() == reign::State::stopped);
 const auto start = reign::monotonic_ns();
 check(reign::monotonic_ns() >= start);
 auto root = std::filesystem::temp_directory_path() / ("reign-test-" + std::to_string(getpid()));
 std::filesystem::create_directories(root / "assets");
 std::ofstream(root / "assets" / "sample", std::ios::binary) << "abc";
 std::ofstream(root / "outside") << "hidden";
 std::filesystem::create_symlink(root / "outside", root / "assets" / "escape");
 auto data = reign::read_asset(root / "assets", "sample");
 check(data.size() == 3 && data[0] == 'a' && data[2] == 'c');
 rejects([&] { reign::read_asset(root / "assets", "sample", 2); });
 rejects([&] { reign::read_asset(root / "assets", "../outside"); });
 rejects([&] { reign::read_asset(root / "assets", "escape"); });
 rejects([&] { reign::read_asset(root / "assets", root / "outside"); });
 rejects([&] { reign::read_asset(root / "assets", "missing"); });
 std::filesystem::remove_all(root);
 std::cout << "Platform services: " << (failed ? "FAIL" : "PASS") << " (" << failed << " failures)\n";
 return failed ? 1 : 0;
}
