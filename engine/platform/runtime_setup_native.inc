#include "varsys.h"
#include "runtime_setup_native.h"
namespace Main {VarInteger elapCap;VarInteger triCount;}
namespace NativeEngine {
void InitializeRuntimeVariables(){
 if(VarSys::FindVarItem("sys.buildtype")||VarSys::FindVarItem("sys.elapcap")||VarSys::FindVarItem("sys.tricount"))throw std::logic_error("Frame variables already exist");
 // This target never opens a network transport. Original UI branches on this flag.
 VarSys::CreateInteger("multiplayer.flags.online",0,VarSys::NOEDIT);
 // Select shipped gameplay UI; native runtime does not expose desktop developer panels.
 VarSys::CreateString("sys.buildtype","release",VarSys::NOEDIT);
 VarSys::CreateInteger("sys.elapcap",8,VarSys::DEFAULT,&Main::elapCap);
 VarSys::CreateInteger("sys.tricount",0,VarSys::DEFAULT,&Main::triCount);
}
}
namespace Version {
const char *GetBuildString(){return "Reigncarnated native ARM64; upstream 9ce9da5f21604310a997f0c41e9cd383f5e292c3";}
}
