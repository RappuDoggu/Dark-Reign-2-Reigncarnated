#include "command_slice.hpp"
#include "console_slice.hpp"
#include "reign/command_scopes.hpp"
#include "reign/engine_file.hpp"
#include <map>
#include <iostream>
namespace {std::vector<std::string> emitted;unsigned called=0;
void emit(FScope *s){emitted.push_back(s->NextArgString());}
void fail(FScope *){throw std::runtime_error("Injected scope failure");}
void command(U32){++called;}}
int main(){try{
 unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("Scope check "+std::to_string(checks));};
 auto rejects=[&](auto f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);};
 struct Session{Session(){TrackSys::Init();VarSys::Init();}~Session(){VarSys::Done();TrackSys::Done();}} session;
 VarSys::CreateInteger("menu.mode",1);VarSys::RegisterHandler("menu",command,VarSys::NOTIFY);VarSys::CreateCmd("menu.go",VarSys::NOTIFY);
 std::map<std::string,std::string> files{
 {"main.cfg",R"(If("menu.mode","==",1){ Emit("if"); } Else() { Emit("else"); }
 Switch("menu.mode"){ Case(1){ Emit("case"); } Default() { Emit("default"); } }
 Cmd("menu.go"); Exec("child.cfg"); ExecOptional("absent.cfg");)"},
 {"child.cfg","Emit(\"child\");"}, {"bad.cfg","Missing();"}, {"emptycmd.cfg","Cmd();"},
 {"loop.cfg","Exec(\"loop.cfg\");"}, {"service.cfg","Bind(\"A\",\"menu.go\");"},
 {"failure.cfg","Switch(\"menu.mode\"){Case(1){Fail();}}"},
 {"percent.cfg","Log(\"Literal %s %n\");"}};
 auto loader=[&](const std::string &name){auto i=files.find(name);if(i==files.end())throw FileSys::FileNotFound(name);return std::vector<U8>(i->second.begin(),i->second.end());};
 reign::CommandScopes scopes(loader);scopes.register_scope("Emit",emit);scopes.register_scope("Fail",fail);
 rejects([&]{scopes.register_scope("Emit",emit);});rejects([&]{scopes.unregister_scope("Emit",fail);});
 reign::run_console([&]{
  check(scopes.execute("main.cfg"));check(emitted==std::vector<std::string>({"if","case","child"})&&called==1);
  emitted.clear();VarSys::FindVarItem("menu.mode")->SetInteger(2);scopes.execute("main.cfg");check(emitted==std::vector<std::string>({"else","default","child"})&&called==2);
  VarSys::FindVarItem("menu.mode")->SetInteger(1);
  rejects([&]{scopes.execute("failure.cfg");});rejects([&]{scopes.execute("bad.cfg");});
  rejects([&]{scopes.execute("emptycmd.cfg");});rejects([&]{scopes.execute("service.cfg");});
  check(!scopes.execute("absent.cfg",false));rejects([&]{scopes.execute("absent.cfg");});
  rejects([&]{scopes.execute("loop.cfg");});check(scopes.execute("child.cfg"));
  check(scopes.execute("percent.cfg"));
 },[](VarSys::VarItem *,U32){});
 unsigned services=0;reign::CommandScopes supported(loader,[&](FScope *s){check(std::string(s->NameStr())=="Bind"&&std::string(s->NextArgString())=="A");++services;});
 check(supported.execute("service.cfg")&&services==1);rejects([&]{supported.execute("child.cfg");});
 scopes.unregister_scope("Emit",emit);rejects([&]{scopes.execute("child.cfg");});scopes.register_scope("Emit",emit);check(scopes.execute("child.cfg"));
 rejects([&]{Main::RegisterRootScope("Outside",emit);});
 VarSys::DeleteItem("menu");VarSys::DeleteItem("console");
 std::cout<<"Original Main conditionals/Exec/registered scopes/console dispatch: PASS ("<<checks<<" checks)\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
