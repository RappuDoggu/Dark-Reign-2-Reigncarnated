#include "reign/runcode_host.hpp"
#include "reign/engine_file.hpp"
#include "fscope.h"
#include "runcodes.h"
#include <iostream>
namespace {
std::vector<std::string> events;bool allow=false;
void ai(){events.push_back("Ai");}void ap(){events.push_back("Ap");}void ad(){events.push_back("Ad");}void ax(){events.push_back("Ax");}
void bi(){events.push_back("Bi");}void bp(){events.push_back("Bp");}void bd(){events.push_back("Bd");}
void qi(){events.push_back("Qi");}void qp(){events.push_back("Qp");}void qd(){events.push_back("Qd");}
void once(){events.push_back("once");}
U32 notify(U32 message){if(message!=0x2f312211u)throw std::runtime_error("Wrong original notification");return allow;}
}
int main(){
 int failures=0;auto check=[&](bool b){if(!b)++failures;};
 reign::RunCodeHost host([](const std::string &name){
  if(name!="runcode_main_A.cfg")throw FileSys::FileNotFound(name);
  std::string text="Note(7);";return std::vector<U8>(text.begin(),text.end());
 },[&](FScope *scope){check(scope->NameCrc()==Crc::CalcStr("Note"));check(scope->NextArgInteger()==7);events.push_back("cfg7");});
 RunCodes rc("main");rc.Register("A",ap,ai,ad,ax,notify);rc.Register("B",bp,bi,bd,nullptr,notify);rc.Register("Quit",qp,qi,qd);
 check(!rc.Set("missing"));check(rc.Set("A"));rc.Process();
 check(events==std::vector<std::string>({"Ai","cfg7","Ax"}));
 rc.Process();check(events.back()=="Ap");rc.Set("B");rc.Process();check(std::string(rc.GetCurrent())=="A");
 allow=true;rc.Set("B");rc.Process();check(std::string(rc.GetCurrent())=="B");
 check(events[events.size()-2]=="Ad" && events.back()=="Bi");
 rc.SetRunOnceProc(once);rc.Process();check(events[events.size()-2]=="once"&&events.back()=="Bp");
 allow=false;rc.Set("Quit");rc.Process();check(events[events.size()-2]=="Bd"&&events.back()=="Qi");
 rc.Clear();check(rc.IsClear());check(events.back()=="Qd");rc.Cleanup();
 std::cout<<"Original run-code lifecycle/config dispatch: "<<(failures?"FAIL":"PASS")<<" ("<<failures<<" failures)\n";return failures?1:0;
}
