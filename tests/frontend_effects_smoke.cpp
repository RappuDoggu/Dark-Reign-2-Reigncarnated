#include "frontend_effects_slice.hpp"
#include "reign/frontend_effects.hpp"
#include <iostream>
#include <stdexcept>
int main(){try{
 unsigned checks=0,calls=0;float volume=0;auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("Effects check "+std::to_string(checks));};
 auto rejects=[&](auto f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);};
 reign::FrontendEffects effects,other;
 rejects([&]{IFace::Sound::Play(1,nullptr);});
 effects.set(1,.25f,[&](float v){++calls;volume=v;});
 effects.run([&]{IFace::Sound::Play(0,nullptr);check(calls==0);IFace::Sound::Play(1,nullptr);check(calls==1&&volume==.25f);
  other.run([&]{IFace::Sound::Play(1,nullptr);});check(calls==1);
  IFace::Sound::Play(1,nullptr);check(calls==2);
 });
 effects.set(2,1,[&](float){++calls;effects.clear(2);});effects.run([&]{IFace::Sound::Play(2,nullptr);});check(calls==3);
 effects.clear(1);effects.run([&]{IFace::Sound::Play(1,nullptr);});check(calls==3);
 rejects([&]{effects.set(1,2,[](float){});});rejects([&]{effects.set(1,1,{});});
 effects.set(255,1,[](float){throw std::runtime_error("Playback failure");});
 rejects([&]{effects.run([]{IFace::Sound::Play(255,nullptr);});});rejects([&]{IFace::Sound::Play(1,nullptr);});
 std::cout<<"Original IFace effect selection/volume/context/failure routing: PASS ("<<checks<<" checks); audio device not tested\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
