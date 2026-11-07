#include "varsys.h"
#include <iostream>
using namespace reign::vars_detail;
namespace {unsigned notifications=0;void changed(U32){++notifications;}
struct Observer:VarNotify {VarPtr &owner;unsigned changes=0,deletes=0;explicit Observer(VarPtr &p):owner(p){}~Observer(){if(owner.Alive())owner.ForgetMe(this);}void Notify(Mode mode)override{if(mode==CHANGED)++changes;else{++deletes;owner.ForgetMe(this);}}};
struct Expansion:VarSys::Substitution {Expansion():Substitution('@'){}~Expansion(){if(node.InUse())VarSys::UnregisterSubstitution(*this);}const char *Expand(const char *,void *context)override{return context?static_cast<const char *>(context):"iface.test";}};
}
int main(){try{
 unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("Var check "+std::to_string(checks));};
 struct Session {bool live=true;Session(){TrackSys::Init();VarSys::Init();}void done(){VarSys::Done();TrackSys::Done();live=false;}~Session(){if(live)done();}} session;
 VarSys::RegisterHandler("iface.test",changed,VarSys::NOTIFY);
 VarInteger value;VarSys::CreateInteger("iface.test.value",10,VarSys::NOTIFY,&value);check(*value==10);VarSys::SetIntegerRange("iface.test.value",0,20);value=50;check(*value==20&&notifications==1);value=-8;check(*value==0);
 Observer observer(value);value.NotifyMe(&observer);value=15;check(observer.changes==1);
 Expansion expansion;VarSys::RegisterSubstitution(expansion);char context[]="iface.test";check(VarSys::FindVarItem("@selected.value",static_cast<void*>(context))==VarSys::FindVarItem("iface.test.value"));VarSys::UnregisterSubstitution(expansion);
 bool badPath=false;try{VarSys::FindVarItem(std::string(256,'a').c_str());}catch(const std::exception &){badPath=true;}check(badPath);
 VarFloat scale;VarSys::CreateFloat("iface.test.scale",.5f,0,&scale);VarSys::SetFloatRange("iface.test.scale",0,1);scale=2;check(*scale==1);
 VarString text;VarSys::CreateString("iface.test.name","Menu",0,&text);check(std::string(*text)=="Menu");text="Dark Reign II";check(std::string(*text)=="Dark Reign II");
 auto *item=VarSys::FindVarItem("iface.test.name");check(item&&item->GetUnicodeStringValue()[0]=='D');
 const U8 bytes[]={1,2,3,4};VarBinary data;VarSys::CreateBinary("iface.test.binary",4,bytes,0,&data);check(VarSys::FindVarItem("iface.test.binary")->BinarySize()==4);VarSys::FindVarItem("iface.test.binary")->SetBinary(8,nullptr);check(VarSys::FindVarItem("iface.test.binary")->Binary()[7]==0);
 check(VarSys::FindVariable("IFACE.TEST.VALUE"));VarSys::CreateCmd("iface.test.action",VarSys::NOTIFY);auto before=notifications;check(VarSys::TriggerCmd("iface.test.action")&&notifications==before+1);
 VarSys::DeleteItem("iface.test");check(observer.deletes==1);check(!value.Alive()&&!text.Alive()&&!scale.Alive()&&!data.Alive());session.done();
 check(std::string(String::ItoA(std::numeric_limits<int>::min(),10))=="-2147483648");check(std::string(String::ItoA(-1,2)).size()==32);
 bool bounded=false;try{String::Make("%s",std::string(600,'x').c_str());}catch(const std::exception &){bounded=true;}check(bounded);
 std::cout<<"Original VarSys scopes/typed values/clamping/notifications/binary/death tracking: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
