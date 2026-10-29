#include "reign/frontend_support.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
int main(){try{
 unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::logic_error("Text check "+std::to_string(checks));};
 CH out[8]={};check(Utils::Ansi2Unicode(out,8,"Menu")==5&&out[0]=='M'&&out[4]==0);
 check(Utils::Ansi2Unicode(out,3,"Menu")==3&&out[1]=='e'&&out[2]==0);
 check(Utils::Ansi2Unicode(out,1,"x")==1&&out[0]==0);
 out[0]=123;bool caught=false;try{Utils::Ansi2Unicode(out,8,"\x80");}catch(const std::runtime_error&){caught=true;}check(caught&&out[0]==123);
 {reign::AnsiBinding binding(reign::AnsiEncoding::windows1252);check(Utils::Ansi2Unicode(out,8,"\x80\x93\xe9")==4);check(out[0]==0x20ac&&out[1]==0x201c&&out[2]==0xe9&&out[3]==0);}
 caught=false;try{Utils::Ansi2Unicode(out,8,"\xe9");}catch(const std::runtime_error&){caught=true;}check(caught);
 char hex[16];check(std::string(Utils::StrFmtHex(hex,8,0xab,0))=="000000AB");
 check(std::string(Utils::StrFmtHex(hex,8,0xab,2))=="    0xAB");
 check(std::string(Utils::StrFmtHex(hex,8,0xab,4))=="00000ABh");
 check(std::string(Utils::StrFmtHex(hex,8,0xab,6))=="   0xABh");
 check(std::string(Utils::StrFmtHex(hex,3,0xa,2))=="  A");
 check(std::string(Utils::StrFmtHex(hex,1,0xab,4))=="h");
 check(std::string(Utils::StrFmtHex(nullptr,8,0xffffffffu,0))=="FFFFFFFF");
 std::cout<<"Frontend ANSI code page/bounds and legacy hexadecimal formatting: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
