#include "reign/config.hpp"
#include "reign/engine_file.hpp"
#include "reign/platform.hpp"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unistd.h>
int main(){
 int failed=0;auto check=[&](bool b){if(!b)++failed;};
 std::unordered_map<std::string,std::string> files={
  {"root.cfg","enum { one=1, two, three }; Value(one, two, three, -42, 0x7FFFFFFF, 101B, 1.25, 50%, \"text\", ''); #include \"child.cfg\""},
  { "limits.cfg", "Limits(0xFFFFFFFF,-2147483648,0x80000000,11111111111111111111111111111111B);" },
  {"child.cfg","Child() { Nested(123); }"},
  {"roundtrip.cfg","Value(1,2,3,-42,0x7FFFFFFF,101B,1.25,50%,\"text\",''); Child() { Nested(123); }"}};
 const auto root=std::filesystem::temp_directory_path()/("reign-config-test-"+std::to_string(getpid()));std::filesystem::create_directory(root);
 reign::ConfigLoader loader=[&](const std::string &name){
  auto it=files.find(name);if(it!=files.end())return std::vector<U8>(it->second.begin(),it->second.end());
  if(std::filesystem::is_regular_file(root/name))return reign::read_asset(root,name);
  throw FileSys::FileNotFound(name);
 };
 try{
  auto limits=reign::parse_config(loader,"limits.cfg");
  check(std::get<std::int32_t>(limits[0].arguments[0])==-1);
  check(std::get<std::int32_t>(limits[0].arguments[1])==(-2147483647-1));
  check(std::get<std::int32_t>(limits[0].arguments[2])==(-2147483647-1));
  check(std::get<std::int32_t>(limits[0].arguments[3])==-1);
  auto tree=reign::parse_config(loader,"root.cfg");
  check(tree.size()==2 && tree[0].name=="Value" && tree[1].name=="Child");
  check(tree[0].arguments.size()==10);
  check(std::get<std::int32_t>(tree[0].arguments[0])==1);
  check(std::get<std::int32_t>(tree[0].arguments[1])==2);
  check(std::get<std::int32_t>(tree[0].arguments[2])==3);
  check(std::get<std::int32_t>(tree[0].arguments[3])==-42);
  check(std::get<std::int32_t>(tree[0].arguments[4])==2147483647);
  check(std::get<std::int32_t>(tree[0].arguments[5])==5);
  check(std::get<float>(tree[0].arguments[6])==1.25f);
  check(std::get<float>(tree[0].arguments[7])==0.5f);
  check(std::get<std::string>(tree[0].arguments[8])=="text");
  check(std::get<std::string>(tree[0].arguments[9]).empty());
  check(tree[1].children.size()==1 && tree[1].children[0].name=="Nested");
  reign::roundtrip_config(loader,"roundtrip.cfg",(root/"out.cfg").string(),(root/"out.blk").string());
  auto text=reign::parse_config(loader,"out.cfg"),binary=reign::parse_config(loader,"out.blk");
  check(text.size()==tree.size() && binary.size()==tree.size());
  check(binary[0].arguments==tree[0].arguments && text[0].arguments==tree[0].arguments);
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';++failed;}
 auto rejects=[&](const std::string &name){try{reign::parse_config(loader,name);++failed;}catch(const std::exception &){}};
 files["missing-include.cfg"]="#include \"does-not-exist.cfg\"";
 files["wide-hex.cfg"]="Value(0x123456789);";
 files["wide-binary.cfg"]="Value(111111111111111111111111111111111B);";
 rejects("missing.cfg");rejects("missing-include.cfg");rejects("wide-hex.cfg");rejects("wide-binary.cfg");
 auto bytes=reign::read_asset(root,"out.blk");
 bytes.resize(47);std::ofstream corrupt(root/"corrupt.blk",std::ios::binary);corrupt.write(reinterpret_cast<const char *>(bytes.data()),bytes.size());corrupt.close();
 rejects("corrupt.blk");
 std::filesystem::remove_all(root);
 std::cout<<"Original config parser/roundtrip: "<<(failed?"FAIL":"PASS")<<" ("<<failed<<" failures)\n";
 return failed?1:0;
}
