#include "babel_slice.hpp"
#include "varfile_slice.hpp"
#include "reign/localization.hpp"
#include <iostream>
#include <string>
#include <vector>
int main(){try{
 unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("Babel check "+std::to_string(checks));};
 auto rejects=[&](auto f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);};
 auto ascii=[](const CH *s){std::string r;while(*s){if(*s>127)throw std::runtime_error("Not ASCII");r+=char(*s++);}return r;};
 auto utf=[](const std::string &s){std::vector<CH> r(s.begin(),s.end());r.push_back(0);return r;};
 check(ascii(Babel::Translate("Main Menu",0))=="Main Menu");
 check(ascii(Babel::Translate("{1:d}",1,-42))=="-42");
 check(ascii(Babel::Translate("{1:08X}",1,-1))=="FFFFFFFF");
 check(ascii(Babel::Translate("{1:u}",1,-1))=="4294967295");
 check(ascii(Babel::Translate("{1:.1f}",1,2.25))=="2.2");
 check(ascii(Babel::Translate("{2:d}/{1:d}",2,7,9))=="9/7");
 const CH name[]={'F','o','l','d',0};
 check(ascii(Babel::Translate("[{1:6.2s}]",1,name))=="[    Fo]");
 check(ascii(Babel::Translate("[{1:-6s}]",1,name))=="[Fold  ]");
 check(ascii(Babel::Translate("{1:c}",1,'A'))=="A");
 check(ascii(Babel::Translate("{{literal}",0))=="{{literal}");
 check(ascii(Babel::Translate("{{{1:d}",1,3))=="{{3");
 check(ascii(Babel::Translate("{1:s}",1,Babel::Translate("Nested",0)))=="Nested");
 const CH unicode[]={0x20ac,0xd83d,0xde00,0};const CH *translated=Babel::Translate("{1:s}",1,unicode);
 check(translated[0]==0x20ac&&translated[1]==0xd83d&&translated[2]==0xde00&&translated[3]==0);
 const CH *slots[5];for(int i=0;i<5;++i)slots[i]=Babel::Translate("{1:d}",1,i);
 for(int i=0;i<5;++i)check(slots[i][0]=='0'+i);const CH *wrapped=Babel::Translate("again",0);check(wrapped==slots[0]);
 CH out[5]={};auto four=utf("four");Babel::Sprintf(out,5,four.data(),0);check(ascii(out)=="four");
 rejects([&]{Babel::Sprintf(out,4,four.data(),0);});
 for(const auto *bad:{"{1:d}{1:d}","{2:d}","{0:d}","{1:*d}","{1:ld}","{1:n}","{1:p}","{1:501d}","{1:d", "{x:d}","{1:q}"})
  rejects([&]{Babel::Translate(bad,2,1,2);});
 rejects([&]{Babel::Translate(nullptr,0);});
 rejects([&]{Babel::Translate(std::string(16384,'a').c_str(),0);});
 rejects([&]{Babel::Translate("{1:s}",1,static_cast<const CH*>(nullptr));});
 auto huge=utf(std::string(600,'a'));rejects([&]{Babel::Translate("{1:s}",1,huge.data());});
 struct Session {Session(){TrackSys::Init();VarSys::Init();}~Session(){VarSys::Done();TrackSys::Done();}} session;
 FScope db(nullptr,"DataBase");auto *value=db.AddFunction("VarBinary");value->AddArgString("menu");value->AddFunction("Size")->AddArgInteger(8);value->AddFunction("Value")->AddArgString("10000000F400B400");db.InitIterators();VarFile::Load(&db,"multilanguage.english");
 reign::Localization lang("english");lang.run([&]{check(ascii(Babel::Translate("#menu",0))=="OK");check(ascii(Babel::Translate("#missing",0))=="Unknown[missing]");});
 rejects([&]{Babel::Translate("#menu",0);});VarSys::DeleteItem("multilanguage");
 std::cout<<"Original Babel substitution/native formatting/bounds/rotation: PASS ("<<checks<<" checks)\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
