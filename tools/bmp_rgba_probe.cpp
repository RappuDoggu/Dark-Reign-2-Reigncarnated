#include "reign/bitmap_loader.hpp"
#include <array>
#include <iostream>
int main(){try{
 std::vector<std::uint8_t> bytes;std::array<char,8192> chunk;
 while(std::cin){std::cin.read(chunk.data(),chunk.size());const auto n=std::cin.gcount();if(bytes.size()+n>128u*1024*1024)throw std::runtime_error("BMP input exceeds probe limit");bytes.insert(bytes.end(),chunk.data(),chunk.data()+n);}
 if(!std::cin.eof())throw std::runtime_error("BMP input read failed");
 auto image=reign::load_bitmap_bmp(bytes);std::cout.write(reinterpret_cast<const char *>(image.pixels.data()),image.pixels.size());if(!std::cout)throw std::runtime_error("RGBA output write failed");
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
