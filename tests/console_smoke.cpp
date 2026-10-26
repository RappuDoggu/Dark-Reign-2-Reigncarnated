#include "console_slice.hpp"
#include <iostream>
namespace {struct Output {unsigned calls=0;U32 last=0;};Bool output(const CH *,U32 &type,void *context){auto &o=*static_cast<Output *>(context);++o.calls;o.last=type;return TRUE;}
unsigned calls=0;int got=0;std::string text;bool nested=false;unsigned argc_seen=0;std::string raw_offset;
void handler(U32 crc){
 ++calls;argc_seen=Console::ArgCount();
 if(crc==Crc::CalcStr("test.raw")){const char *s=nullptr;Console::GetArgString(1,s);text=s?s:"";raw_offset=Console::GetCmdString(1);return;}
 if(crc==Crc::CalcStr("test.fail"))throw std::runtime_error("handler failure");
 if(crc==Crc::CalcStr("test.nested")){
  auto before=std::string(Console::GetCmdString());Console::ProcessCmd("test.capture 17 inside");
  nested=before==Console::GetCmdString();return;
 }
 S32 value=0;if(Console::GetArgInteger(1,value))got=value;
 const char *s=nullptr;if(Console::GetArgString(2,s))text=s;
}}
int main(){try{
 unsigned checks=0,displays=0;auto check=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Console check "+std::to_string(checks));};
 auto rejects=[&](auto f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);};
 struct Session{Session(){TrackSys::Init();VarSys::Init();}~Session(){VarSys::Done();TrackSys::Done();}} session;
 VarSys::RegisterHandler("test",handler,VarSys::NOTIFY);
 for(auto *name:{"test.capture","test.nested","test.fail"})VarSys::CreateCmd(name,VarSys::NOTIFY);
 VarSys::CreateCmd("test.raw",VarSys::NOTIFY|VarSys::RAWDATA);
 VarSys::CreateCmd("test.rawvar",VarSys::NOTIFY|VarSys::RAWVAR);
 VarSys::CreateInteger("test.number",2);VarSys::CreateString("test.name","Menu");
 auto display=[&](VarSys::VarItem *item,U32){check(item!=nullptr);++displays;};
 rejects([&]{Console::ProcessCmd("test.capture 1");});
 reign::run_console([&]{
  Console::ProcessCmd("test.capture 42 \"hello world\"");check(calls==1&&got==42&&text=="hello world");
  check(!VarSys::FindVarItem("console.stack1"));
  Console::ProcessCmd("test.number = 7; test.name = Updated");check(VarSys::FindVarItem("test.number")->Integer()==7&&std::string(VarSys::FindVarItem("test.name")->Str())=="Updated"&&displays==2);
  Console::ProcessCmd("test.capture test.number test.name");check(got==7&&text=="Updated");
  Console::ProcessCmd("test.rawvar 1 test.name");check(text=="test.name");
  Console::ProcessCmd("test.raw untouched; rest");check(text==raw_offset&&text.find("untouched; rest")!=std::string::npos&&argc_seen==2);
  Console::ProcessCmd("test.capture -2.75 float");check(got==-2);
  Console::ProcessCmd("test.nested");check(nested&&got==17&&text=="inside");
  rejects([&]{Console::ProcessCmd("test.fail 1 2");});check(!VarSys::FindVarItem("console.stack1"));
  rejects([&]{Console::ProcessCmd("test.capture 1 =");});check(!VarSys::FindVarItem("console.stack1"));
  rejects([&]{Console::ProcessCmd("unknown.command");});
  rejects([&]{Console::ProcessCmd("test.number = 1000000000000000000000000000000.0");});
  Console::ProcessCmd("test.capture 8 recovered");check(got==8&&text=="recovered");
  rejects([&]{Console::ProcessCmd(nullptr);});
 },display);
 rejects([&]{Console::ProcessCmd("test.capture 1");});
 rejects([&]{reign::run_console([]{},{});});
 reign::run_console([&]{
  Output seen;BinTree<U32> filter;filter.Add(7);
  Console::RegisterMsgHook(&filter,output,&seen);
  rejects([&]{Console::RegisterMsgHook(nullptr,output,&seen);});
  Console::AddString("Visible",7);Console::AddString("Filtered",9);check(seen.calls==1&&seen.last==7);
  Console::UnregisterMsgHook(output,&seen);rejects([&]{Console::UnregisterMsgHook(output,&seen);});
  seen.calls=0;Console::EnumStrings(&filter,output,&seen);check(seen.calls==1);
  for(unsigned i=0;i<120;++i)Console::AddString("Ring",i);
  seen.calls=0;Console::EnumStrings(nullptr,output,&seen);check(seen.calls==100&&seen.last==119);
  Console::ClearScrollBack();seen.calls=0;Console::EnumStrings(nullptr,output,&seen);check(seen.calls==0);filter.DisposeAll();
 },display);
 VarSys::DeleteItem("test");VarSys::DeleteItem("console");
 std::cout<<"Original console parsing/assignment/arguments/dispatch/unwind: PASS ("<<checks<<" checks)\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
