#include "varsys.h"
#include "runtime_setup_native.h"
namespace Main {VarInteger elapCap;VarInteger triCount;}
namespace NativeEngine {
void InitializeRuntimeVariables(){
 if(VarSys::FindVarItem("sys.elapcap")||VarSys::FindVarItem("sys.tricount"))throw std::logic_error("Frame variables already exist");
 VarSys::CreateInteger("sys.elapcap",8,VarSys::DEFAULT,&Main::elapCap);
 VarSys::CreateInteger("sys.tricount",0,VarSys::DEFAULT,&Main::triCount);
}
}
namespace Version {
const char *GetBuildString(){return "Reigncarnated native ARM64; upstream 9ce9da5f21604310a997f0c41e9cd383f5e292c3";}
}
