#pragma once
#include <functional>
#include <string>
namespace reign {
struct SaveSlotInfo {bool valid=false;std::string description;};
struct SaveMission {
 std::string name;bool system=true;
 struct Name {const char *str;};
 Name GetName()const{return {name.c_str()};}
 bool IsSystem()const{return system;}
};
// The host must bind real save-game services before activating this dialog.
// Missing callbacks throw. This boundary does not implement mission serialization.
struct SaveLoadServices {
 std::function<SaveSlotInfo(const std::string &)> info;
 std::function<bool(const std::string &,const std::string &)> save;
 std::function<bool(const std::string &)> load;
 // Returned mission must remain valid throughout event dispatch.
 std::function<const SaveMission *()> active_mission;
 std::function<std::string()> date_time;
 void run(const std::function<void()> &)const;
 static void register_class();
};
namespace save_load_detail {
using Info=SaveSlotInfo;using Mission=SaveMission;
bool GetInfo(const char *,Info &);
bool SlotUsed(const char *);
bool Save(const char *,const char *);
bool Load(const char *);
const Mission *GetActive();
const char *DateTime();
}
}
