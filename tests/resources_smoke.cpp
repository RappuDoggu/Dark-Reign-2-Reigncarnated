#include "reign/resources.hpp"
#include "reign/engine_file.hpp"
#include <fstream>
#include <iostream>
#include <unistd.h>
int main(){
 int failures=0;auto check=[&](bool b){if(!b)++failures;};
 auto rejects=[&](auto op){try{op();++failures;}catch(const std::exception &){}};
 auto root=std::filesystem::temp_directory_path()/("reign-resources-"+std::to_string(getpid()));
 std::filesystem::create_directories(root/"Library/Engine");
 std::filesystem::create_directories(root/"client");std::filesystem::create_directories(root/"base");
 std::ofstream(root/"Library/Engine/Startup.CFG")<<"startup";
 std::ofstream(root/"client/shared.cfg")<<"client";std::ofstream(root/"base/shared.cfg")<<"base";
 std::ofstream(root/"base/extra.cfg")<<"extra";
 reign::ResourceStore store(root);
 check(store.add_directory("client","client"));check(store.add_directory("base","base"));
 store.add_stream("game","client");store.add_stream("game","base");
 auto a=store.load("GAME","SHARED.CFG");check(std::string(a.begin(),a.end())=="client");
 auto b=store.load("game","extra.cfg");check(std::string(b.begin(),b.end())=="extra");
 auto c=store.load("any","library\\engine\\startup.cfg");check(std::string(c.begin(),c.end())=="startup");
 check(!store.add_archive("base","missing.zwp"));
 rejects([&]{store.load("game","../secret");});rejects([&]{store.load("game","/etc/passwd");});
 rejects([&]{store.load("game","missing.cfg");});
 store.add_stream("loop1","loop2");store.add_stream("loop2","loop1");rejects([&]{store.load("loop1","missing.cfg");});
 std::filesystem::remove_all(root);
 std::cout<<"Resource precedence/path checks: "<<(failures?"FAIL":"PASS")<<" ("<<failures<<" failures)\n";return failures?1:0;
}
