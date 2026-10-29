#include "varfile_slice.hpp"
#include <iostream>
int main(){try{
 unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("VarFile check "+std::to_string(checks));};
 auto rejects=[&](auto f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);};
 struct Session {Session(){TrackSys::Init();VarSys::Init();}~Session(){VarSys::Done();TrackSys::Done();}} session;
 U8 bytes[3]={9,9,9};Utils::CharDecode(bytes,"21BA00",6,3);check(bytes[0]==0x12&&bytes[1]==0xab&&bytes[2]==0);
 rejects([&]{Utils::CharDecode(bytes,"123",3,3);});rejects([&]{Utils::CharDecode(bytes,"12",2,0);});rejects([&]{Utils::CharDecode(bytes,"GG",2,3);});
 rejects([&]{Utils::CharDecode(bytes,"ab",2,3);});
 FScope root(nullptr,"Database");auto *nested=root.AddFunction("VarScope");nested->AddArgString("menu");
 auto *value=nested->AddFunction("VarInteger");value->AddArgString("count");value->AddArgInteger(7);
 auto *text=nested->AddFunction("VarString");text->AddArgString("name");text->AddArgString("Shell");
 auto *binary=nested->AddFunction("VarBinary");binary->AddArgString("caption");binary->AddFunction("Size")->AddArgInteger(8);binary->AddFunction("Value")->AddArgString("10000000");binary->AddFunction("Value")->AddArgString("14000000");
 root.InitIterators();VarFile::Load(&root,"language");
 check(VarSys::FindVarItem("language.menu.count")->Integer()==7);check(std::string(VarSys::FindVarItem("language.menu.name")->Str())=="Shell");
 auto *item=VarSys::FindVarItem("language.menu.caption");check(item->BinarySize()==8&&item->Binary()[0]==1&&item->Binary()[4]=='A');
 FScope update(nullptr,"Database");auto *changed=update.AddFunction("VarInteger");changed->AddArgString("count");changed->AddArgInteger(9);update.InitIterators();VarFile::Load(&update,"language.menu");check(VarSys::FindVarItem("language.menu.count")->Integer()==9);
 FScope overflow(nullptr,"Database");auto *bad=overflow.AddFunction("VarBinary");bad->AddArgString("bad");bad->AddFunction("Size")->AddArgInteger(1);bad->AddFunction("Value")->AddArgString("1234");overflow.InitIterators();rejects([&]{VarFile::Load(&overflow,"language");});
 FScope huge(nullptr,"Database");auto *tooBig=huge.AddFunction("VarBinary");tooBig->AddArgString("huge");tooBig->AddFunction("Size")->AddArgInteger(64*1024*1024+1);huge.InitIterators();rejects([&]{VarFile::Load(&huge,"language");});
 FScope longPath(nullptr,"Database");longPath.AddFunction("VarScope")->AddArgString(std::string(255,'x').c_str());longPath.InitIterators();rejects([&]{VarFile::Load(&longPath,"language");});
 VarSys::DeleteItem("language");
 std::cout<<"Original variable database/nibble order/bounds/nesting/update: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
