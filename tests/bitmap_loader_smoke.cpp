#include "reign/bitmap_loader.hpp"
#include "reign/tga_source.hpp"
#include <iostream>
#include <stdexcept>
int main(){try{
 unsigned checks=0;
 auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("check "+std::to_string(checks));};
 auto header=[](unsigned type,unsigned depth,unsigned w=2,unsigned h=1){std::vector<std::uint8_t> b(18);b[2]=type;b[12]=w;b[14]=h;b[16]=depth;b[17]=32;return b;};
 auto reject=[&](auto b){bool failed=false;try{reign::load_bitmap_tga(b);}catch(const std::exception &){failed=true;}check(failed);};
 auto b=header(2,16);b.insert(b.end(),{0,252,31,0});
 auto i=reign::load_bitmap_tga(b);check(i.pixels==std::vector<std::uint8_t>({248,0,0,128,0,0,248,0}));
 b=header(1,8);b[1]=1;b[3]=7;b[5]=2;b[7]=32;b.insert(b.end(),{1,2,3,0,4,5,6,123,7,8});
 check(reign::load_bitmap_tga(b).pixels==std::vector<std::uint8_t>({3,2,1,255,6,5,4,255}));
 auto bad=b;bad.back()=9;reject(bad);bad=b;bad[3]=255;reject(bad);
 auto p16=header(1,8);p16[1]=1;p16[5]=1;p16[7]=16;p16.insert(p16.end(),{31,0,0,0});
 check(reign::load_bitmap_tga(p16).pixels==std::vector<std::uint8_t>({248,0,0,255,248,0,0,255}));
 // RLE packet crosses a scanline; all four origin combinations map to one image.
 for(unsigned origin=0;origin<4;++origin){
  auto r=header(10,24,2,2);r[17]=origin<<4;r.insert(r.end(),{3});
  const std::uint8_t colors[4][3]={{0,0,255},{0,255,0},{255,0,0},{255,255,255}};
  for(unsigned y=0;y<2;++y)for(unsigned x=0;x<2;++x){auto dx=(origin&1)?1-x:x,dy=(origin&2)?y:1-y;const auto *c=colors[dy*2+dx];r.insert(r.end(),c,c+3);}
  check(reign::load_bitmap_tga(r).pixels==std::vector<std::uint8_t>({255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,255}));
  for(std::size_t n=0;n<r.size();++n)reject(std::vector<std::uint8_t>(r.begin(),r.begin()+n));
 }
 auto r=header(9,8);r[1]=1;r[5]=1;r[7]=24;r.insert(r.end(),{0,0,255,129,0});
 check(reign::load_bitmap_tga(r).pixels==std::vector<std::uint8_t>({255,0,0,255,255,0,0,255}));
 auto overflow=r;overflow[21]=130;reject(overflow);
 for(std::size_t n=0;n<r.size();++n)reject(std::vector<std::uint8_t>(r.begin(),r.begin()+n));
 auto gray=header(3,8);gray.insert(gray.end(),{0,1});reject(gray);
 auto interleave=r;interleave[17]|=64;reject(interleave);
 auto id=b;id[0]=3;id.insert(id.begin()+18,{11,22,33});check(reign::load_bitmap_tga(id).pixels==reign::load_bitmap_tga(b).pixels);
 // PIC raw RGB and per-scanline separate alpha streams.
 auto pic=[](unsigned width,bool alpha,unsigned encoding){std::vector<std::uint8_t> p(104);p[0]=0x53;p[1]=0x80;p[2]=0xf6;p[3]=0x34;p[88]='P';p[89]='I';p[90]='C';p[91]='T';p[92]=width>>8;p[93]=width;p[95]=1;p.insert(p.end(),{std::uint8_t(alpha),8,std::uint8_t(encoding),0xe0});if(alpha)p.insert(p.end(),{0,8,std::uint8_t(encoding),0x10});return p;};
 auto q=pic(2,true,0);q.insert(q.end(),{255,0,0,0,255,0,255,64});
 check(reign::load_bitmap_pic(q).pixels==std::vector<std::uint8_t>({255,0,0,255,0,255,0,64}));
 auto reject_pic=[&](auto bytes){bool failed=false;try{reign::load_bitmap_pic(bytes);}catch(const std::exception &){failed=true;}check(failed);};
 for(std::size_t n=0;n<q.size();++n)reject_pic(std::vector<std::uint8_t>(q.begin(),q.begin()+n));
 auto mixed=pic(3,true,2);mixed.insert(mixed.end(),{129,255,0,0,0,0,255,0,2,255,128,0});
 check(reign::load_bitmap_pic(mixed).pixels==std::vector<std::uint8_t>({255,0,0,255,255,0,0,128,0,255,0,0}));
 auto longrun=pic(130,false,2);longrun.insert(longrun.end(),{128,0,130,0,0,255});
 auto li=reign::load_bitmap_pic(longrun);check(li.pixels.size()==520&&li.pixels[2]==255&&li.pixels[518]==255);
 auto zero=longrun;zero[110]=0;reject_pic(zero);auto excess=longrun;excess[110]=131;reject_pic(excess);
 auto unknown=q;unknown[106]=1;reject_pic(unknown);auto chain=q;chain[108]=1;reject_pic(chain);
 std::cout<<"TGA/PIC source rows -> original Bitmap::CopyLine: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
