#include "reign/frontend_prelude.hpp"
#include <iostream>
namespace {U32 gotId=0,gotContext=0;void callback(U32 id,U32 context){gotId=id;gotContext=context;}}
int main(){try{
 unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("Message event check "+std::to_string(checks));};
 CH text[]={'O','K',0};
 {MBEvent event("Accept",text);check(event.caption!=text);check(event.caption[0]=='O'&&event.caption[2]==0);text[0]='X';check(event.caption[0]=='O');check(event.ident.crc==Crc::CalcStr("Accept"));}
 {MBEventCallback event("Cancel",text,callback,0xfedcba98);event.Process();check(gotId==Crc::CalcStr("Cancel")&&gotContext==0xfedcba98);}
 // A dead target must not submit an event or require IFace startup.
 {MBEventNotify event("Dead",text,nullptr,123);event.Process();check(true);}
 bool rejected=false;try{MBEventCallback event("Bad",text,nullptr,0);}catch(const std::exception&){rejected=true;}check(rejected);
 std::cout<<"Original message-box caption ownership/callback/dead target: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
