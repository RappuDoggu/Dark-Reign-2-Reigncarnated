#include "reign/archive.hpp"
#include "reign/bitmap_manager.hpp"
#include <iostream>
int main(int argc,char **argv){try{
 if(argc!=2)throw std::runtime_error("Usage: bitmap_manager_probe installation/base.zwp");reign::Archive archive(argv[1]);unsigned reads=0,requests=0;
 reign::BitmapManager manager([&](const std::string &n){++reads;return archive.read(n);},[&](const std::string &n){return archive.find(n)!=nullptr;});
 for(const auto &e:archive.entries())if(e.name.size()>4){const auto ext=e.name.substr(e.name.size()-4);if(ext!=".tga"&&ext!=".pic"&&ext!=".bmp")continue;auto bmp=manager.load(e.name);if(!bmp||!bmp->resident||bmp->texture)throw std::runtime_error("CPU manager load failed: "+e.name);if(bmp->source!=e.name)std::cout<<"Original cache reused: requested="<<e.name<<" resident="<<bmp->source<<'\n';++requests;}
 const auto entries=manager.count(),initialReads=reads;manager.release();manager.restore();if(reads!=initialReads+entries)throw std::runtime_error("Reload count differs from tree count");
 std::cout<<"Original Bitmap manager real assets: requests="<<requests<<" managed_entries="<<entries<<" initial_reads="<<initialReads<<" restored_entries="<<reads-initialReads<<" CPU-only PASS\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
