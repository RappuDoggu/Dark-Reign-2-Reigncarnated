#include "reign/archive.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>
namespace {
void u32(std::vector<std::uint8_t> &b, std::size_t p, std::uint32_t v) {
 for (int i=0; i<4; ++i) b[p+i]=static_cast<std::uint8_t>(v>>(8*i));
}
std::vector<std::uint8_t> fixture() {
 std::vector<std::uint8_t> b(85,0);
 b[0]='N'; b[1]='O'; b[2]='R'; b[3]='K';
 u32(b,4,1);u32(b,8,1);u32(b,12,80);u32(b,16,1);u32(b,20,59);
 b[56]='a';b[57]=0;b[58]='c';u32(b,59,1);b[63]=8;
 const std::string name="base.cfg";std::copy(name.begin(),name.end(),b.begin()+64);
 u32(b,72,56);u32(b,76,3);b[80]=4;b[81]='d';b[82]='a';b[83]='t';b[84]='a';return b;
}
}
int main() {
 int failed=0;
 auto check=[&](bool b) { if (!b) ++failed; };
 auto root=std::filesystem::temp_directory_path()/("reign-pack-test-"+std::to_string(getpid()));
 std::filesystem::create_directory(root);const auto file=root/"fixture.zwp";
 auto write=[&](const auto &b) { std::ofstream out(file,std::ios::binary);out.write(reinterpret_cast<const char *>(b.data()),b.size()); };
 auto b=fixture();write(b);reign::Archive archive(file);
 check(archive.entries().size()==1);check(archive.find("BASE.CFG")!=nullptr);check(!archive.find("missing.cfg"));
 check(!archive.find(std::string("base.cfg\0extra",14)));
 auto data=archive.read("BASE.CFG");check(data.size()==3 && data[0]=='a' && data[1]==0 && data[2]=='c');
 auto rejects=[&](auto operation) { try { operation();++failed; } catch(const std::exception &) {} };
 rejects([&] { archive.read("base.cfg",2); });
 rejects([&] { archive.read("missing.cfg"); });
 for(std::size_t size=0;size<b.size();++size) {
  write(std::vector<std::uint8_t>(b.begin(),b.begin()+size));
  rejects([&] { reign::Archive invalid(file); });
 }
 for(auto [offset,value]:std::vector<std::pair<std::size_t,std::uint32_t>>{
  {0,0},{4,2},{8,99999},{12,0xffffffffu},{16,99999},{20,0},{59,2},{72,0},{76,0xffffffffu}}) {
  auto bad=b;u32(bad,offset,value);write(bad);rejects([&] { reign::Archive invalid(file); });
 }
 // Two case-equivalent records must not silently shadow in the same archive.
 auto duplicate=b;
 duplicate.insert(duplicate.begin()+80,b.begin()+59,b.begin()+80);
 u32(duplicate,12,101);u32(duplicate,16,2);
 write(duplicate);rejects([&] { reign::Archive invalid(file); });
 write(b);std::filesystem::resize_file(file,57);
 rejects([&] { archive.read("base.cfg"); });
 check(reign::name_crc("Intro")==0xa521e37bu);
 std::filesystem::remove_all(root);
 std::cout << "Native PACK checks: " << (failed?"FAIL":"PASS") << " (" << failed << " failures)\n";
 return failed?1:0;
}
