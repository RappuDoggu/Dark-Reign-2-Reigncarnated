#include "reign/bitmap_manager.hpp"
#include <algorithm>
#include <iostream>
#include <map>
#include <set>
std::vector<std::uint8_t> tga(unsigned r=255,unsigned g=0){std::vector<std::uint8_t>b(18);b[2]=2;b[12]=b[14]=1;b[16]=24;b[17]=32;b.insert(b.end(),{0,std::uint8_t(g),std::uint8_t(r)});return b;}
std::vector<std::uint8_t> pic(){std::vector<std::uint8_t>b(104);b[0]=0x53;b[1]=0x80;b[2]=0xf6;b[3]=0x34;b[88]='P';b[89]='I';b[90]='C';b[91]='T';b[93]=b[95]=1;b.insert(b.end(),{0,8,0,0xe0,0,255,0});return b;}
std::string lower(std::string s){std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return std::tolower(c);});return s;}
int main(){try{
 unsigned checks=0,reads=0,uploads=0,destroys=0,unbinds=0;bool fail=false;std::set<unsigned> live;
 auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("Manager check "+std::to_string(checks));};
 auto rejects=[&](auto f){bool threw=false;try{f();}catch(const std::exception &){threw=true;}check(threw);};
 std::map<std::string,std::vector<std::uint8_t>> files{{"hero.tga",tga()},{"hero.pic",pic()},{"prefer.pic",pic()},{"prefer.tga",tga()},{"walk.tga",tga()},{"walk1.tga",tga(0,255)},{"walk2.tga",tga()},{"gap.tga",tga()},{"gap3.tga",tga()},{"fail.tga",tga()},{"fail1.tga",{1}},{"only.bmp",{1}},{"dir\\odd.tga",tga()}};
 auto loader=[&](const std::string &n){++reads;return files.at(lower(n));};auto exists=[&](const std::string &n){return files.count(lower(n))!=0;};
 reign::BitmapGpuCallbacks gpu{[&](const auto &){if(fail)throw std::runtime_error("upload failed");auto id=++uploads;live.insert(id);return id;},[&](unsigned id){++destroys;if(!live.erase(id))std::terminate();},[&]{++unbinds;}};
 reign::BitmapManager manager(loader,exists,gpu);
 auto hero=manager.load("hero.tga");check(hero&&hero->resident&&hero->texture&&hero->image.pixels[0]==255);
 check(manager.load("HERO.pic")==hero&&manager.find("hero")==hero&&reads==1);check(manager.animation("hero").size()==1);
 auto preferred=manager.load("prefer.xyz");check(preferred->source=="prefer.pic"&&preferred->image.pixels[1]==255);
 auto walk=manager.load("walk.tga");auto frames=manager.animation("walk");check(frames.size()==3&&frames[0]==walk&&frames[1]->frame==1&&frames[2]->frame==2&&frames[1]->image.pixels[1]==255);
 check(manager.load("gap.tga")&&manager.animation("gap").size()==1);check(!manager.load("noextension")&&!manager.load("missing.tga"));
 const auto count=manager.count(),oldReads=reads;manager.release();check(live.empty()&&!hero->resident&&!hero->texture&&hero->image.pixels[0]==255);
 check(manager.load("hero.tga")==hero&&!hero->resident&&reads==oldReads);
 manager.restore();check(live.size()==count&&hero->resident&&hero->generation==2&&reads==oldReads+count);auto after=reads;manager.restore();check(reads==after);
 const auto destroyBefore=destroys,unbindBefore=unbinds;live.clear();manager.abandon_context();check(destroys==destroyBefore&&unbinds==unbindBefore&&!hero->resident&&!hero->texture);
 manager.restore();check(hero->generation==3&&live.size()==count);
 rejects([&]{manager.load("fail.tga");});check(manager.count()==count&&!manager.find("fail")&&live.size()==count);
 rejects([&]{manager.load("only.bmp");});check(manager.count()==count);
 files["upload.tga"]=tga();fail=true;rejects([&]{manager.load("upload.tga");});check(manager.count()==count);fail=false;check(manager.load("upload.tga")!=nullptr);
 auto odd=manager.load("dir\\odd.tga");check(odd&&!manager.find("dir\\odd.tga")); // Retain original Setup/Find key mismatch for directory-qualified names.
 reign::BitmapManager independent(loader,exists);auto cpu=independent.load("hero.tga");check(cpu&&cpu->resident&&!cpu->texture&&cpu!=hero);
 rejects([&]{manager.load(std::string(241,'a'));});rejects([&]{manager.load(std::string("a\0.tga",6));});
 manager.clear();check(manager.count()==0&&live.empty()&&!hero->resident&&hero->image.pixels.size()==4);
 reign::BitmapManager *self=nullptr;reign::BitmapManager reentrant([&](const std::string &){self->find("hero");return tga();},exists);self=&reentrant;rejects([&]{reentrant.load("hero.tga");});check(reentrant.count()==0);
 unsigned probes=0;reign::BitmapManager disappearing(loader,[&](const std::string &){return ++probes<3;});rejects([&]{disappearing.load("hero.tga");});check(disappearing.count()==0);
 std::cout<<"Original Bitmap manager fallback/cache/animation/release/restore/ownership: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
