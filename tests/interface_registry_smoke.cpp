#include "iface_events.hpp"
#include "reign/interface_events.hpp"
#include <iostream>
#include <memory>
namespace {
// Registry tests never instantiate a control; any accidental factory call fails.
IControl *unused_factory(U32,IControl *,U32){throw std::logic_error("Unexpected factory invocation");}
}
int main(){try{
 unsigned checks=0;
 auto check=[&](bool ok){++checks;if(!ok)throw std::logic_error("Registry check "+std::to_string(checks));};
 auto rejects=[&](auto call){bool caught=false;try{call();}catch(const std::runtime_error&){caught=true;}check(caught);};
 auto definition=[](const char *name,const char *base){
  auto scope=std::make_unique<FScope>(nullptr,"DefineControlType");
  scope->AddArgString(name);scope->AddArgString(base);
  scope->AddFunction("Width")->AddArgInteger(640);scope->InitIterators();return scope;
 };
 reign::InterfaceEvents one,two;
 one.run([&]{
  check(IFace::RootWindow()==nullptr&&IFace::OverlaysWindow()==nullptr&&IFace::GameWindow()==nullptr);
  check(!reign::iface_event_detail::state().treeReady);
  {
   FScope input(nullptr,"RegistryData");input.AddArgString("ShellPanel");
   input.AddFunction("CreateControl")->AddArgString("Shell");input.InitIterators();
   reign::iface_event_detail::load_registry(&input);
   auto *copy=IFace::FindRegData("ShellPanel",FALSE,FALSE);check(copy&&copy!=&input);
   check(std::string(copy->NextArgString())=="ShellPanel");
  }
  auto *stored=IFace::FindRegData("ShellPanel",FALSE,FALSE);
  check(stored&&std::string(stored->NextArgString())=="ShellPanel");
  auto *childScope=IFace::FindRegData("ShellPanel",TRUE,FALSE);
  check(childScope&&std::string(childScope->NameStr())=="CreateControl");
  check(std::string(childScope->NextArgString())=="Shell");
  childScope=IFace::FindRegData("ShellPanel",TRUE,FALSE);
  check(std::string(childScope->NextArgString())=="Shell");
  FScope duplicateScope(nullptr,"RegistryData");duplicateScope.AddArgString("ShellPanel");
  duplicateScope.AddFunction("Replacement");duplicateScope.InitIterators();
  reign::iface_event_detail::load_registry(&duplicateScope);
  check(IFace::FindRegData("ShellPanel",FALSE,FALSE)==stored);
  check(std::string(IFace::FindRegData("ShellPanel",TRUE,FALSE)->NameStr())=="CreateControl");
  check(!IFace::FindRegData("Absent",FALSE,FALSE));
  check(!IFace::FindRegData("Absent",TRUE,TRUE));
  FScope emptyScope(nullptr,"RegistryData");emptyScope.AddArgString("Empty");emptyScope.InitIterators();
  reign::iface_event_detail::load_registry(&emptyScope);
  check(IFace::FindRegData("Empty",FALSE,FALSE)&&!IFace::FindRegData("Empty",TRUE,FALSE));
  two.run([&]{check(!IFace::FindRegData("ShellPanel",FALSE,FALSE));});
  bool missingScope=false;try{reign::iface_event_detail::load_registry(nullptr);}
  catch(const std::invalid_argument&){missingScope=true;}check(missingScope);
  auto key=Crc::CalcStr("Button");check(IFace::FindControlClass(key)==nullptr);
  // Original BinTree Add returns FALSE for a successful, nonduplicate insert.
  check(IFace::RegisterControlClass("Button",unused_factory)==FALSE);
  ICClass *base=IFace::FindControlClass(key);
  check(base&&base->createProc==unused_factory&&!base->derived&&!base->scope);
  rejects([&]{IFace::RegisterControlClass("Button",unused_factory);});
  check(IFace::FindControlClass(key)==base);
  two.run([&]{check(!IFace::FindControlClass(key));IFace::RegisterControlClass("Button",unused_factory);});
  check(IFace::FindControlClass(key)==base);
  {
   auto scope=definition("MenuButton","Button");IFace::DefineControlType(scope.get());
   auto *derived=IFace::FindControlClass(Crc::CalcStr("MenuButton"));
   check(derived&&derived->derived&&derived->base.crc==key);
   check(derived->createProc==unused_factory&&derived->scope!=scope.get());
   scope->GetFunction("Width")->AddArgInteger(999);
  }
  auto *derived=IFace::FindControlClass(Crc::CalcStr("MenuButton"));
  derived->scope->InitIterators();auto *width=derived->scope->GetFunction("Width");
  width->InitIterators();check(width->NextArgInteger()==640);check(width->NextArgument(FALSE)==nullptr);
  auto duplicate=definition("MenuButton","Button");
  rejects([&]{IFace::DefineControlType(duplicate.get());});
  check(IFace::FindControlClass(Crc::CalcStr("MenuButton"))==derived);
  check(reign::iface_event_detail::state().ctrlTypes.GetCount()==2);
  auto missing=definition("Bad","Absent");rejects([&]{IFace::DefineControlType(missing.get());});
  check(!IFace::FindControlClass(Crc::CalcStr("Bad")));
  auto nested=definition("Nested","MenuButton");IFace::DefineControlType(nested.get());
  auto *child=IFace::FindControlClass(Crc::CalcStr("Nested"));
  check(child&&child->base.crc==derived->type.crc&&child->createProc==unused_factory);
  check(IFace::UnregisterControlClass("MenuButton"));check(!IFace::FindControlClass(Crc::CalcStr("MenuButton")));
  rejects([&]{IFace::UnregisterControlClass("MenuButton");});
  check(IFace::FindControlClass(key)==base);
  // Remaining base and derived scopes are owned and freed by session shutdown.
 });
 two.run([&]{check(IFace::FindControlClass(Crc::CalcStr("Button"))!=nullptr);});
 rejects([]{IFace::FindControlClass(0);});
 std::cout<<"Original IFace registry, template ownership and isolation: PASS ("<<checks<<" checks)\n";
 std::cout<<"Control creation and main menu: not exercised\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
