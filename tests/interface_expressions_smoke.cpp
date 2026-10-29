#include "control_types.hpp"
#include "operation.h"
#include "comparison.h"
#include "switch.h"
#include <iostream>
#include <memory>
#include <functional>
int main(){try{
 unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("Expression check "+std::to_string(checks));};
 struct Session{Session(){TrackSys::Init();VarSys::Init();}~Session(){VarSys::Done();TrackSys::Done();}} session;
 VarInteger count,other;VarFloat scale;VarString name;
 VarSys::CreateInteger("menu.count",4,0,&count);VarSys::CreateInteger("menu.other",3,0,&other);
 VarSys::CreateFloat("menu.scale",.5f,0,&scale);VarSys::CreateString("menu.name","Login",0,&name);
 auto expr=[](const char *var,const char *op=nullptr){auto s=std::make_unique<FScope>(nullptr,"Test");s->AddArgString(var);if(op)s->AddArgString(op);return s;};
 auto operate=[&](auto &s){s->InitIterators();Operation::Function(s.get(),nullptr);};
 auto compare=[&](auto &s){s->InitIterators();return Comparison::Function(s.get(),nullptr);};
 {auto s=expr("menu.count","+");s->AddArgInteger(2);operate(s);check(*count==6);}
 {auto s=expr("menu.count","*");s->AddArgString("menu.other");operate(s);check(*count==18);}
 {auto s=expr("menu.scale","+");s->AddArgFPoint(.25f);operate(s);check(*scale==.75f);}
 {auto s=expr("menu.name","=");s->AddArgString("Shell");operate(s);check(std::string(*name)=="Shell");}
 {auto s=expr("menu.count",">");s->AddArgInteger(10);check(compare(s));}
 {auto s=expr("menu.count","==");s->AddArgString("menu.other");check(!compare(s));}
 {auto s=expr("menu.count");check(compare(s));count=0;check(!compare(s));}
 {auto s=expr("menu.scale","<");s->AddArgInteger(1);check(compare(s));}
 {auto s=expr("menu.name","==");s->AddArgString("Shell");check(compare(s));}
 for(const char *key:{"menu.count","menu.scale","menu.name"}){
  auto s=expr(key);s->InitIterators();auto *v=&Switch::GetValue(s.get(),nullptr);
  std::unique_ptr<Switch::Value,void(*)(Switch::Value*)> owned(v,[](auto *p){Switch::DisposeValue(*p);});
  auto test=std::make_unique<FScope>(nullptr,"Case");
  if(std::string(key)=="menu.count")test->AddArgInteger(0);else if(std::string(key)=="menu.scale")test->AddArgFPoint(.75f);else test->AddArgString("Shell");
  test->InitIterators();check(Switch::CompareValue(*v,test.get(),nullptr));
 }
 auto rejects=[&](std::function<void()> f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);};
 {auto s=expr("menu.missing","+");s->AddArgInteger(1);rejects([&]{operate(s);});}
 {auto s=expr("menu.count","+");rejects([&]{operate(s);});}
 {auto s=expr("menu.count","???");s->AddArgInteger(1);rejects([&]{compare(s);});}
 VarSys::DeleteItem("menu");
 std::cout<<"Original menu operations/comparisons/switches: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
