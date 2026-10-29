#include "control_types.hpp"
#include "fscope.h"
#include "control_stdload.hpp"
#include <iostream>
#include <functional>
#include <memory>

int main(){try {
 unsigned checks=0;
 auto check=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Frontend config check "+std::to_string(checks));};
 auto rejects=[&](std::function<void()> f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);};
 auto scope=[](){return std::make_unique<FScope>(nullptr,"MenuTest");};
 {auto s=scope();s->AddArgInteger(42);s->InitIterators();check(StdLoad::TypeU32(s.get())==42);}
 {auto s=scope();s->AddArgInteger(-1);s->InitIterators();check(StdLoad::TypeU32(s.get())==0xffffffffu);}
 {auto s=scope();check(StdLoad::TypeU32(s.get(),7u)==7);}
 {auto s=scope();s->AddArgInteger(2);s->InitIterators();rejects([&]{StdLoad::TypeU32(s.get(),Range<U32>::flag);});}
 {auto s=scope();rejects([&]{StdLoad::TypeU32(s.get(),2u,Range<U32>::flag);});}
 {auto s=scope();s->AddArgString("wrong");s->InitIterators();rejects([&]{StdLoad::TypeU32(s.get(),0u);});}
 {auto s=scope();s->AddArgFPoint(.5f);s->InitIterators();check(StdLoad::TypeF32(s.get(),Range<F32>::percentage)==.5f);}
 {auto s=scope();s->AddArgInteger(3);s->InitIterators();check(StdLoad::TypeF32(s.get(),0.f)==3.f);}
 {auto s=scope();check(StdLoad::TypeF32(s.get(),.25f)==.25f);}
 {auto s=scope();s->AddArgFPoint(1.5f);s->InitIterators();rejects([&]{StdLoad::TypeF32(s.get(),Range<F32>::percentage);});}
 {auto s=scope();s->AddArgString("Shell");s->InitIterators();check(std::string(StdLoad::TypeString(s.get()))=="Shell");}
 {auto s=scope();check(std::string(StdLoad::TypeStringD(s.get(),"Login"))=="Login");}
 {auto s=scope();s->AddArgInteger(3);s->InitIterators();rejects([&]{StdLoad::TypeStringD(s.get(),"");});}
 {auto s=scope();s->AddArgString("Shell");s->InitIterators();check(StdLoad::TypeStringCrc(s.get())==0x5B31647Eu);}
 {auto s=scope();check(StdLoad::TypeStringCrc(s.get(),123u)==123u);}
 {auto s=scope();s->AddArgInteger(-20);s->AddArgInteger(55);s->InitIterators();Point<S32> p;StdLoad::TypePoint(s.get(),p);check(p.x==-20&&p.y==55);}
 {auto s=scope();s->AddArgFPoint(.25f);s->AddArgFPoint(.75f);s->InitIterators();Point<F32> p;StdLoad::TypePoint(s.get(),p);check(p.x==.25f&&p.y==.75f);}
 {auto s=scope();s->AddArgInteger(-1);s->AddArgInteger(2);s->InitIterators();Point<U32> p;StdLoad::TypePoint(s.get(),p);check(p.x==0xffffffffu&&p.y==2);}
 {auto s=scope();s->AddFunction("Width")->AddArgInteger(640);s->InitIterators();check(StdLoad::TypeU32(s.get(),"Width")==640);}
 {auto s=scope();check(StdLoad::TypeU32(s.get(),"Missing",7u)==7);}
 {auto s=scope();rejects([&]{StdLoad::TypeString(s.get(),"Missing");});}
 {auto s=scope();check(std::string(StdLoad::TypeString(s.get(),"Missing","Login"))=="Login");}
 {auto s=scope();check(StdLoad::TypeStringCrc(s.get(),"Missing",321u)==321u);}
 // Preserve the original distinction: absent named scope returns its default
 // directly, while a present scope's absent argument validates that default.
 {auto s=scope();check(StdLoad::TypeU32(s.get(),"Missing",2u,Range<U32>::flag)==2u);}
 std::cout<<"Original menu StdLoad scalar/string/CRC/point paths: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
