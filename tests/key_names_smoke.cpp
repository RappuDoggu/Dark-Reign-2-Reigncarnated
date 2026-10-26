#include "key_names_slice.hpp"
#include "reign/key_names.hpp"
#include <iostream>
int main(){try{
 unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("Key-name check "+std::to_string(checks));};
 auto rejects=[&](auto f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);};
 reign::KeyNames names,other;
 auto key=[&](int value,const char *name){FScope s(nullptr,"Scancodes");auto *k=s.AddFunction("Key");k->AddArgInteger(value);k->AddArgString(name);s.InitIterators();names.configure(&s);};
 key(30,"A");key(255,"Last");check(names.key_count()==2&&names.scan_code("a")==30&&names.scan_code("Last")==255);
 rejects([&]{key(256,"Overflow");});rejects([&]{key(-1,"Negative");});rejects([&]{key(30,"Duplicate");});rejects([&]{key(31,"A");});rejects([&]{key(31,std::string(32,'a').c_str());});
 rejects([&]{other.scan_code("A");});
 FScope mods(nullptr,"Modifiers");mods.AddFunction("Shift")->AddArgString("SHIFT");mods.AddFunction("LeftCtrl")->AddArgString("LC");mods.InitIterators();names.configure(&mods);
 check(names.modifier_count()==2&&names.modifier_flags("shift")==(Input::LSHIFTDOWN|Input::RSHIFTDOWN|Input::SHIFTDOWN));check(names.modifier_flags("LC")==Input::LCTRLDOWN);
 mods.InitIterators();rejects([&]{names.configure(&mods);});
 FScope bad(nullptr,"Modifiers");bad.AddFunction("NotAModifier")->AddArgString("bad");bad.InitIterators();rejects([&]{names.configure(&bad);});
 check(names.key_count()==2&&names.modifier_count()==2);
 const char *found="unchanged";names.run([&]{check(reign::key_name_detail::FindKeyByScan(30,found)&&std::string(found)=="A");check(!reign::key_name_detail::FindKeyByScan(256,found));other.run([&]{check(!reign::key_name_detail::FindKeyByScan(30,found));});check(reign::key_name_detail::FindKeyByScan(255,found)&&std::string(found)=="Last");});rejects([&]{reign::key_name_detail::FindKeyByScan(30,found);});
 std::cout<<"Original key-name/modifier table and 256 boundary: PASS ("<<checks<<" checks); key events not tested\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
