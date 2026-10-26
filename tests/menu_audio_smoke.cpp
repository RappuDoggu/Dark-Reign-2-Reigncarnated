#include "reign/menu_audio.hpp"
#include <iostream>
#include <limits>
#include <fstream>
#include <chrono>
int main(){try{
 unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("Audio check "+std::to_string(checks));};
 std::vector<std::uint8_t> wav={'R','I','F','F',40,0,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,1,0,1,0,0x40,0x1f,0,0,0x80,0x3e,0,0,2,0,16,0,'d','a','t','a',4,0,0,0,0,0,0,0};
 auto root=std::filesystem::temp_directory_path()/("reign-audio-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 struct Clean{std::filesystem::path p;~Clean(){std::filesystem::remove_all(p);}}clean{root};
 unsigned loads=0;std::function<void(float)> expired;
 {reign::MenuAudio audio([&](const std::string&){++loads;return wav;},root);auto a=audio.load("../../supplied-name.wav");auto b=audio.load("../../supplied-name.wav");check(loads==1);auto events=audio.drain();check(events.size()==1&&events[0].volume==-1);check(std::filesystem::path(events[0].path).parent_path()==root);std::ifstream f(events[0].path,std::ios::binary);std::vector<std::uint8_t> disk((std::istreambuf_iterator<char>(f)),{});check(disk==wav);a(.1f);b(1);a(0);events=audio.drain();check(events.size()==2&&events[0].volume==.1f&&events[1].volume==1);check(audio.drain().empty());
 for(float v:{-1.f,1.1f,std::numeric_limits<float>::quiet_NaN()}){bool fail=false;try{a(v);}catch(const std::invalid_argument&){fail=true;}check(fail);}
 for(int i=0;i<64;++i)a(.5f);bool full=false;try{a(.5f);}catch(const std::runtime_error&){full=true;}check(full);check(audio.drain().size()==64);expired=a;}
 bool dead=false;try{expired(1);}catch(const std::runtime_error&){dead=true;}check(dead);
 for(unsigned change=0;change<5;++change){auto bad=wav;if(change==0)bad.resize(20);if(change==1)bad[20]=2;if(change==2)bad[40]=255;if(change==3)bad[32]=0;if(change==4)bad[4]=0;reign::MenuAudio audio([&](const std::string&){return bad;},root);bool fail=false;try{audio.load("bad");}catch(const std::runtime_error&){fail=true;}check(fail);check(audio.drain().empty());}
 std::cout<<"Menu audio cache, PCM validation, volume, bounded delivery and expired callbacks: PASS ("<<checks<<" checks)\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
