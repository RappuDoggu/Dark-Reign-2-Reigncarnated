#pragma once
#include "reign/config.hpp"
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>
namespace reign {
struct MenuSoundEvent { std::string path; float volume; }; // -1 preloads only
// Serialized with the menu on its render thread. Callbacks expire with the bank.
class MenuAudio {
 struct State;std::shared_ptr<State> state_;
public:
 MenuAudio(ConfigLoader,const std::filesystem::path &cache);
 std::function<void(float)> load(const std::string &);
 std::vector<MenuSoundEvent> drain();
 std::string cached_path(const std::string &);
};
}
