#include "reign/bink_movie.hpp"
#include <iostream>
int main(){unsigned rejected=0;for(unsigned n:{0u,8u,44u,128u}){std::vector<std::uint8_t> bytes(n);if(n>=3){bytes[0]='B';bytes[1]='I';bytes[2]='K';}try{reign::BinkMovie invalid(std::move(bytes));}catch(...){++rejected;}}if(rejected!=4)return 1;std::cout<<"Invalid/truncated Bink headers rejected before decoding: PASS (4)\n";}
