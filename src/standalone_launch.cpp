#include "console_slice.hpp"
#include "reign/menu_commands.hpp"
namespace {
void standalone_login(U32 crc) {
 if(crc!=Crc::CalcStr("multiplayer.login")||Console::ArgCount()!=1)
  throw std::invalid_argument("Only standalone multiplayer.login without arguments is supported");
 auto *flag=VarSys::FindVarItem("multiplayer.flags.isLobby");
 if(!flag||flag->Integer()!=0)throw std::runtime_error("Lobby launch is not implemented");
 // Inspected upstream multiplayer_cmd.cpp: all login/network effects require
 // CmdLine::host or CmdLine::join. This adapter exposes neither. Their reset
 // therefore leaves standalone state unchanged, including the selected user.
}
}
namespace reign {
void register_standalone_launch_commands(){
 VarSys::RegisterHandler("multiplayer",standalone_login,VarSys::NOTIFY);
 VarSys::CreateInteger("multiplayer.flags.isLobby",0);
 VarSys::CreateCmd("multiplayer.login",VarSys::NOTIFY);
}
}
