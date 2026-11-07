#include "game_saveload.h"
#include "reign/save_load_services.hpp"
namespace {
thread_local const reign::SaveLoadServices *active=nullptr;
const reign::SaveLoadServices &services(){if(!active)throw std::runtime_error("Save-game services not bound");return *active;}
template<class F> const F &required(const F &fn,const char *name){if(!fn)throw std::runtime_error(std::string("Save-game service missing: ")+name);return fn;}
IControl *factory(U32,IControl *parent,U32){return new Game::SaveLoad(parent);}
}
namespace reign {
void SaveLoadServices::register_class(){IFace::RegisterControlClass("Game::SaveLoad",factory);}
void SaveLoadServices::run(const std::function<void()> &call)const{if(!call)throw std::invalid_argument("Missing save-game dispatch");auto *previous=active;active=this;struct Restore{const SaveLoadServices *p;~Restore(){active=p;}} restore{previous};call();}
namespace save_load_detail {
bool GetInfo(const char *slot,Info &out){if(!slot)throw std::invalid_argument("Missing save slot");out=required(services().info,"info")(slot);return out.valid;}
bool SlotUsed(const char *slot){Info out;return GetInfo(slot,out);}
bool Save(const char *slot,const char *description){if(!slot||!description)throw std::invalid_argument("Missing save arguments");return required(services().save,"save")(slot,description);}
bool Load(const char *slot){if(!slot)throw std::invalid_argument("Missing save slot");return required(services().load,"load")(slot);}
const Mission *GetActive(){return required(services().active_mission,"active mission")();}
const char *DateTime(){thread_local std::string text;text=required(services().date_time,"date/time")();return text.c_str();}
}
}
