#include "localization_slice.hpp"
#include "varfile_slice.hpp"
#include "reign/localization.hpp"
#include <iostream>
int main(){try {
 unsigned checks=0;
 auto check=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Localization check "+std::to_string(checks));};
 auto rejects=[&](auto call){bool caught=false;try{call();}catch(const std::exception&){caught=true;}check(caught);};
 struct Session{Session(){TrackSys::Init();VarSys::Init();}~Session(){VarSys::Done();TrackSys::Done();}} session;
 auto load=[&](const char *language,const char *name,const char *hex,int size){
  FScope db(nullptr,"DataBase");auto *v=db.AddFunction("VarBinary");v->AddArgString(name);
  v->AddFunction("Size")->AddArgInteger(size);v->AddFunction("Value")->AddArgString(hex);
  db.InitIterators();VarFile::Load(&db,(std::string("multilanguage.")+language).c_str());
 };
 load("english","caption","7000000014002400",8); // version7, A B; no terminator
 load("other","caption","900000003400",6);
 load("english","short","0000",2);load("english","odd","0000000000",5);
 load("english","surrogate","00000000008D",6);
 load("english","pair","00000000D38D00ED",8); // D83D DE00
 VarSys::CreateInteger("multilanguage.english.integer",1);
 reign::Localization english("english"),other("other"),missing("absent");
 const CH *data=nullptr;U32 bytes=0,version=0;
 rejects([&]{MultiLanguage::Lookup("caption",data,bytes);});
 english.run([&]{
  MultiLanguage::Lookup("caption",data,bytes,version);check(bytes==4&&version==7&&data[0]=='A'&&data[1]=='B');
  other.run([&]{MultiLanguage::Lookup("caption",data,bytes,version);check(bytes==2&&version==9&&data[0]=='C');});
  MultiLanguage::Lookup("caption",data,bytes);check(bytes==4&&data[0]=='A');
  rejects([&]{other.run([]{throw std::runtime_error("unwind");});});
  MultiLanguage::Lookup("caption",data,bytes);check(bytes==4&&data[0]=='A');
  MultiLanguage::Lookup("missing",data,bytes,version);check(!data&&bytes==0&&version==0);
  MultiLanguage::Lookup("",data,bytes,version);check(!data&&bytes==0&&version==0);
  rejects([&]{MultiLanguage::Lookup(nullptr,data,bytes);});
  rejects([&]{MultiLanguage::Lookup(std::string(300,'x').c_str(),data,bytes);});
  for(const auto *key:{"short","odd","surrogate","integer"})rejects([&]{MultiLanguage::Lookup(key,data,bytes);});
  MultiLanguage::Lookup("pair",data,bytes);check(bytes==4&&data[0]==0xd83d&&data[1]==0xde00);
 });
 missing.run([&]{MultiLanguage::Lookup("caption",data,bytes);check(!data&&bytes==0);});
 rejects([&]{MultiLanguage::Lookup("caption",data,bytes);});
 rejects([&]{reign::Localization bad("english.other");});rejects([&]{reign::Localization bad("");});
 rejects([&]{english.run({});});
 VarSys::DeleteItem("multilanguage");
 std::cout<<"Original localization lookup/context/UTF16/bounds: PASS ("<<checks<<" checks)\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
