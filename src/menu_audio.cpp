#include "reign/menu_audio.hpp"
#include <cmath>
#include <fstream>
#include <map>
#include <stdexcept>
#include <cstring>
namespace {
unsigned u16(const std::vector<std::uint8_t>&b,std::size_t p){return b[p]|(unsigned(b[p+1])<<8);}
unsigned u32(const std::vector<std::uint8_t>&b,std::size_t p){return u16(b,p)|(u16(b,p+2)<<16);}
void validate(const std::vector<std::uint8_t>&b){
 if(b.size()<12||b.size()>1024*1024||std::memcmp(b.data(),"RIFF",4)||std::memcmp(b.data()+8,"WAVE",4)||u32(b,4)!=b.size()-8)throw std::runtime_error("Invalid menu WAV container");
 bool fmt=false,data=false;unsigned align=0,rate=0;std::size_t samples=0;
 for(std::size_t p=12;p<b.size();){
  if(b.size()-p<8)throw std::runtime_error("Truncated menu WAV chunk");auto n=u32(b,p+4);auto start=p+8;
  if(n>b.size()-start)throw std::runtime_error("Truncated menu WAV data");
  if(!std::memcmp(b.data()+p,"fmt ",4)){
   if(fmt||n<16||u16(b,start)!=1)throw std::runtime_error("Menu audio requires PCM WAV");
   unsigned channels=u16(b,start+2),bits=u16(b,start+14);rate=u32(b,start+4);align=u16(b,start+12);
   if((channels!=1&&channels!=2)||(bits!=8&&bits!=16)||rate<8000||rate>48000||align!=channels*bits/8||u32(b,start+8)!=rate*align)throw std::runtime_error("Unsupported menu PCM format");fmt=true;
  }else if(!std::memcmp(b.data()+p,"data",4)){if(data||!n)throw std::runtime_error("Invalid menu PCM payload");data=true;samples=n;}
  auto padded=std::size_t(n)+(n&1u);if(padded>b.size()-start)throw std::runtime_error("Missing menu WAV padding");p=start+padded;
 }
 if(!fmt||!data||samples%align||samples/align>rate*5u)throw std::runtime_error("Invalid or oversized menu PCM samples");
}
}
namespace reign {
struct MenuAudio::State {ConfigLoader loader;std::filesystem::path cache;std::map<std::string,std::string> files;std::vector<MenuSoundEvent> events;};
MenuAudio::MenuAudio(ConfigLoader loader,const std::filesystem::path &cache):state_(std::make_shared<State>()){
 if(!loader)throw std::invalid_argument("Missing menu sound loader");state_->loader=std::move(loader);std::filesystem::create_directories(cache);state_->cache=cache;
}
std::function<void(float)> MenuAudio::load(const std::string &name){
 auto &s=*state_;auto found=s.files.find(name);
 if(found==s.files.end()){
  if(s.files.size()>=32)throw std::runtime_error("Too many menu sounds");auto bytes=s.loader(name);validate(bytes);
  auto path=s.cache/("effect-"+std::to_string(s.files.size())+".wav");auto temp=path;temp+=".tmp";
  {std::ofstream out(temp,std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());out.close();if(!out){std::filesystem::remove(temp);throw std::runtime_error("Cannot cache menu WAV");}}
  std::filesystem::rename(temp,path);found=s.files.emplace(name,path.string()).first;s.events.push_back({found->second,-1});
 }
 auto path=found->second;std::weak_ptr<State> weak=state_;
 return [weak,path](float volume){auto s=weak.lock();if(!s)throw std::runtime_error("Menu sound bank closed");if(!std::isfinite(volume)||volume<0||volume>1)throw std::invalid_argument("Invalid menu sound volume");if(volume==0)return;if(s->events.size()>=64)throw std::runtime_error("Menu sound queue full");s->events.push_back({path,volume});};
}
std::string MenuAudio::cached_path(const std::string &name){load(name);return state_->files.at(name);}
std::vector<MenuSoundEvent> MenuAudio::drain(){std::vector<MenuSoundEvent> result;result.swap(state_->events);return result;}
}
