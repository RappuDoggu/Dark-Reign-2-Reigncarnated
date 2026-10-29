#include "cursor_slice.hpp"
#include "reign/cursors.hpp"
#include <iostream>
#include "reign/frontend_bitmap.hpp"
#include "reign/bitmap_manager.hpp"
int main(){try{
 unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("Cursor check "+std::to_string(checks));};
 auto rejects=[&](auto fn){bool caught=false;try{fn();}catch(const std::exception &){caught=true;}check(caught);};
 reign::Cursors cursors,other;
 auto create=[&](const char *name,const char *kind){FScope s(nullptr,"CreateCursor");s.AddArgString(name);s.AddArgString(kind);s.InitIterators();cursors.configure(&s);};
 create("None","Null");create("Alias","None");check(cursors.count()==2);check(cursors.select("Alias"));
 rejects([&]{create("None","Null");});check(cursors.count()==2);
 rejects([&]{create("Bad","Unknown");});rejects([&]{other.select("None");});
 FScope standards(nullptr,"StandardCursors");standards.AddFunction("Default")->AddArgString("None");standards.InitIterators();cursors.configure(&standards);check(cursors.select(nullptr));
 cursors.draw(4,5,0xffffffffu);check(other.count()==0);
 rejects([&]{cursors.configure(nullptr);});
 std::vector<std::uint8_t> pixels(18);pixels[2]=2;pixels[12]=4;pixels[14]=2;pixels[16]=24;pixels[17]=32;for(unsigned i=0;i<8;++i)pixels.insert(pixels.end(),{0,0,255});
 reign::BitmapManager manager([&](const std::string &){return pixels;},[](const std::string &n){return n=="fixture.tga";});reign::FrontendTextures textures(manager);
 textures.run([&]{
 FScope c(nullptr,"CreateCursor");c.AddArgString("Animated");c.AddArgString("Bitmap");
 for(int x:{0,2}){auto *frame=c.AddFunction("AddFrame");auto *t=frame->AddFunction("Texture");t->AddArgString("fixture.tga");for(int v:{x,0,2,2})t->AddArgInteger(v);auto *h=frame->AddFunction("Hotspot");h->AddArgInteger(1);h->AddArgInteger(1);}
 c.InitIterators();cursors.configure(&c);check(cursors.select("Animated"));
 float firstU=0;unsigned batches=0;
 auto draw=[&](unsigned ms){reign::FrontendDraw::run_batches([&]{cursors.draw(10,20,ms);},[&](const auto &batch){check(batch.texture!=nullptr&&batch.vertices.size()==4);if(!batches)firstU=batch.vertices[0].u;else if(batches==1)check(batch.vertices[0].u!=firstU);++batches;});};
 draw(0);draw(101);draw(0xffffffffu);check(batches==3);
 });
 std::cout<<"Original cursor creation/derived selection/ownership: PASS ("<<checks<<" checks)\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
