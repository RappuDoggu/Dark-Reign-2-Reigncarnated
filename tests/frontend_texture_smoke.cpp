#include "reign/frontend_draw.hpp"
#include "reign/frontend_bitmap.hpp"
#include "reign/bitmap_manager.hpp"
#include "fscope.h"
#include <iostream>
#include <map>
namespace IFace {void FScopeToTextureInfo(FScope *,TextureInfo &);}
std::vector<std::uint8_t> tga(){std::vector<std::uint8_t>b(18);b[2]=2;b[12]=4;b[14]=2;b[16]=24;b[17]=32;for(unsigned i=0;i<8;++i)b.insert(b.end(),{0,0,255});return b;}
int main(){try{
 unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("Texture config check "+std::to_string(checks));};
 auto rejects=[&](auto f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);};
 std::map<std::string,std::vector<std::uint8_t>> files{{"menu.tga",tga()},{"anim.tga",tga()},{"anim1.tga",tga()},{"dir\\odd.tga",tga()}};
 reign::BitmapManager manager([&](const auto &n){return files.at(n);},[&](const auto &n){return files.count(n)!=0;});
 reign::FrontendTextures textures(manager);TextureInfo oldInfo,modern;
 rejects([&]{Bitmap::Manager::FindCreate(0,"menu.tga");});
 textures.run([&]{
  FScope s(nullptr,"Texture");s.AddArgString("menu.tga");s.InitIterators();IFace::FScopeToTextureInfo(&s,oldInfo);
  check(oldInfo.texture&&oldInfo.texture->Width()==4&&oldInfo.texture->Height()==2);
  check(oldInfo.pixels.p1.x==4&&oldInfo.uv.p1.x==1&&oldInfo.texMode==TextureInfo::TM_STRETCHED&&!oldInfo.filter);
  check(oldInfo.texture->surface().pixels().pixels[0]==255);
  check(Bitmap::Manager::FindCreate(0,"menu.tga")==oldInfo.texture);
  FScope config(nullptr,"Texture");auto *image=config.AddFunction("Image");image->AddArgString("menu.tga");for(int v:{1,0,2,1})image->AddArgInteger(v);
  config.AddFunction("Mode")->AddArgString("Tile");config.AddFunction("Filter")->AddArgInteger(1);config.InitIterators();IFace::FScopeToTextureInfo(&config,modern);
  check(modern.texture==oldInfo.texture&&modern.texMode==TextureInfo::TM_TILED&&modern.filter);
  check(modern.pixels.p0.x==1&&modern.pixels.p1.x==3&&modern.pixels.p1.y==1);
  check(modern.uv.p0.x==(1+oldInfo.texture->UVShiftWidth())/4&&modern.uv.p1.y==(1+oldInfo.texture->UVShiftHeight())/2);
  rejects([&]{Bitmap::Manager::FindCreate(1,"menu.tga");});
  rejects([&]{Bitmap::Manager::FindCreate(0,"anim.tga");});
  rejects([&]{Bitmap::Manager::FindCreate(0,"dir\\odd.tga");});
  FScope missing(nullptr,"Texture");missing.AddArgString("missing.tga");missing.InitIterators();TextureInfo fail;
  rejects([&]{IFace::FScopeToTextureInfo(&missing,fail);});
 });
 rejects([&]{Bitmap::Manager::FindCreate(0,"menu.tga");});
 manager.release();textures.run([&]{rejects([&]{Bitmap::Manager::FindCreate(0,"menu.tga");});});
 manager.restore();textures.run([&]{auto *fresh=Bitmap::Manager::FindCreate(0,"menu.tga");check(fresh!=oldInfo.texture);check(oldInfo.texture->surface().pixels().pixels[0]==255);});
 std::cout<<"Original UI texture configuration/native manager snapshots: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
