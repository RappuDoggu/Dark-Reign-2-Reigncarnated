#include "music_native.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#define check(x) do{if(!(x))throw std::runtime_error("Music check failed at line "+std::to_string(__LINE__));}while(0)
struct Player:NativeEngine::MusicPlayer{
 bool active=false,accept=true;float volume=0;int starts=0,stops=0;std::filesystem::path path;
 bool Start(const std::filesystem::path &p)override{++starts;path=p;return active=accept;}
 void Stop()noexcept override{++stops;active=false;}
 bool Playing()const noexcept override{return active;}
 void SetVolume(float v)noexcept override{volume=v;}
};
int main(){
 char pattern[]="/tmp/reign-music-XXXXXX";const char *temp=mkdtemp(pattern);if(!temp)return 1;
 struct Cleanup{std::filesystem::path root;~Cleanup(){NativeEngine::RemoveMusicPlayer();std::filesystem::remove_all(root);}}cleanup{temp};
 try{
 using namespace Sound::Redbook;
 auto a=cleanup.root/"Track02.ogg",b=cleanup.root/"Track04.ogg";
 std::ofstream(a)<<"fixture";std::ofstream(b)<<"fixture";
 check(!Claimed()&&!Play(2)&&!TrackCount());
 auto p=std::make_shared<Player>();NativeEngine::InstallMusicPlayer(p,{{4,b},{2,a}});
 check(Claimed()&&TrackCount()==4&&Volume()==1);
 Release();check(!Claimed()&&!Play(2));check(Claim());
 SetEnabled(0);check(!GetEnabled()&&!Play(2));SetEnabled(1);check(GetEnabled());
 check(Play(2)==2&&p->path==a);Poll();check(p->starts==1);
 p->active=false;Poll();check(CurrentTrack()==4&&p->path==b);
 p->active=false;Poll();check(CurrentTrack()==2);
 check(Play(3)==2&&Play(99)==2);
 SetVolume(2);check(p->volume==1);SetVolume(-1);check(p->volume==0);
 auto stops=p->stops;bool rejected=false;
 try{NativeEngine::InstallMusicPlayer(p,{{2,a},{2,b}});}catch(const std::invalid_argument &){rejected=true;}
 check(rejected&&p->stops==stops&&CurrentTrack()==2);
 p->accept=false;check(!Play(4)&&!CurrentTrack());
 auto starts=p->starts;Poll();check(p->starts==starts);
 SetMinTrack(5);check(!Play(4));Stop();NativeEngine::RemoveMusicPlayer();
 check(!Claimed()&&!TrackCount()&&!CurrentTrack()&&!Play(2));
 std::cout<<"Imported music numbering, wrap, completion, volume and failure propagation pass; fixture player, no audible playback.\n";
 return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
