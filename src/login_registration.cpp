#include "game_login.h"
#include "reign/profile_storage.hpp"
namespace {IControl *login(U32,IControl *parent,U32){return new Game::Login(parent);}}
namespace reign {void ProfileStorage::register_login_class(){IFace::RegisterControlClass("Game::Login",login);}}
