#pragma once
#include <cstdint>
#include <memory>
#include <vector>
namespace reign {
// Bounded in-memory decoder for the owner's original Bink files.
class BinkMovie {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 explicit BinkMovie(std::vector<std::uint8_t>);
 ~BinkMovie();
 unsigned width()const;unsigned height()const;unsigned sample_rate()const;
 double duration()const;
 std::int64_t frame_index()const;
 bool advance(double seconds); // false after the final frame's display interval
 void restore_position(double seconds); // fresh decoder only; discard preceding PCM
 const std::vector<std::uint8_t>& rgba()const;
 std::vector<std::int16_t> drain_audio(); // interleaved stereo PCM, original rate
};
}
