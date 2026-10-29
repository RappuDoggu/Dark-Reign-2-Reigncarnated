#include "music_native.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace {
std::shared_ptr<NativeEngine::MusicPlayer> player;
std::vector<NativeEngine::MusicTrack> tracks;
std::uint32_t current=0,minimum=2;
float volume=1;
bool enabled=true,claimed=true;
}
namespace NativeEngine {
void InstallMusicPlayer(std::shared_ptr<MusicPlayer> next,std::vector<MusicTrack> list){
 if(!next||list.empty())throw std::invalid_argument("Music needs a player and imported tracks");
 std::sort(list.begin(),list.end(),[](const auto &a,const auto &b){return a.number<b.number;});
 std::uint32_t previous=0;
 for(const auto &track:list){
  if(!track.number||track.number>99||track.number==previous||!std::filesystem::is_regular_file(track.path))
   throw std::invalid_argument("Invalid, duplicate, or missing imported music track");
  previous=track.number;
 }
 if(player)player->Stop();
 tracks=std::move(list);player=std::move(next);current=0;minimum=2;volume=1;claimed=enabled=true;
 player->SetVolume(volume);
}
void RemoveMusicPlayer()noexcept{if(player)player->Stop();current=0;tracks.clear();player.reset();}
}
namespace Sound { namespace Redbook {
void Stop(){current=0;if(player)player->Stop();}
std::uint32_t Play(std::uint32_t requested){
 if(!player||!enabled||!claimed)return 0;
 const auto first=std::lower_bound(tracks.begin(),tracks.end(),minimum,[](const auto &t,auto n){return t.number<n;});
 if(first==tracks.end()){Stop();return 0;}
 auto selected=std::lower_bound(first,tracks.end(),requested,[](const auto &t,auto n){return t.number<n;});
 // Preserve original out-of-range wrap; missing numbered imports also wrap.
 if(selected==tracks.end()||selected->number!=requested)selected=first;
 Stop();
 if(player->Start(selected->path))current=selected->number;
 return current;
}
std::uint32_t TrackCount(){return player&&!tracks.empty()?tracks.back().number:0;}
std::uint32_t CurrentTrack(){return current;}
void SetMinTrack(std::uint32_t n){minimum=std::max(std::uint32_t(1),n);}
void SetVolume(float v){volume=std::isfinite(v)?std::clamp(v,0.f,1.f):0.f;if(player)player->SetVolume(volume);}
float Volume(){return player?volume:0.f;}
void Poll(){if(player&&current&&!player->Playing()){
 auto next=std::upper_bound(tracks.begin(),tracks.end(),current,[](auto n,const auto &t){return n<t.number;});
 Play(next==tracks.end()?minimum:next->number);
}}
int GetEnabled(){return enabled?1:0;}
void SetEnabled(int value){enabled=value!=0;if(!enabled)Stop();}
int Claim(){claimed=bool(player)&&enabled;return claimed?1:0;}
int Release(){const bool previous=player&&claimed;Stop();claimed=false;return previous?1:0;}
int Claimed(){return player&&claimed?1:0;}
} }
