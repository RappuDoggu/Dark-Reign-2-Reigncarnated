#pragma once
#include "reign/mission_fs.hpp"
#include "reign/sound_settings.hpp"
#include <filesystem>
#include <functional>
#include <memory>
class PTree;
namespace reign {
// Original User state belongs to one session. Campaigns must be initialized and
// outlive this object. Use only an app-private writable directory, never the
// installation directory. run() binds profile IO/audio; close() explicitly saves.
// Destruction discards unflushed memory; it never silently attempts a failing save.
class ProfileStorage {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 using AddonHandler=std::function<void(unsigned,std::uint32_t,void *)>;
 ProfileStorage(const std::filesystem::path &,DisabledAudioSettings &,AddonHandler={});
 ~ProfileStorage();
 ProfileStorage(const ProfileStorage&)=delete;
 ProfileStorage &operator=(const ProfileStorage&)=delete;
 void run(const std::function<void()> &);
 void close();
 static void register_login_class();
};
namespace profile_detail {
bool exists(const char *);bool make_directory(const char *);bool remove_profile(const char *);
void make_path(char *,unsigned,const char *,const char *);
bool write_tree(PTree &,const char *);
void addon(unsigned,std::uint32_t,void *);
}
}
