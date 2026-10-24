#include "reign/config_prelude.hpp"
#include "ptree.h"
#include "blockfile.h"
static_assert(sizeof(BlockHeader)==44 && sizeof(BlockInfo)==8);
static_assert(sizeof(PTree::BinaryInfo)==12 && alignof(PTree)>=alignof(void*));
#include "reign/config.hpp"
namespace reign {
namespace {
std::vector<ConfigNode> snapshot(FScope *scope,unsigned depth=0) {
 if(depth>256)throw std::runtime_error("Config scope nesting exceeds diagnostic limit");
 std::vector<ConfigNode> result;
 scope->InitIterators();
 while(auto *function=scope->NextFunction()) {
  ConfigNode n; n.name=function->NameStr();n.crc=function->NameCrc();
  function->InitIterators();
  while(auto *arg=function->NextArgument()) {
   switch(arg->aType) {
    case VNode::AT_INTEGER:n.arguments.emplace_back(arg->GetInteger());break;
    case VNode::AT_FPOINT:n.arguments.emplace_back(arg->GetFPoint());break;
    case VNode::AT_STRING:n.arguments.emplace_back(std::string(arg->GetString()));break;
    default:throw std::runtime_error("Unsupported snapshot argument type");
   }
  }
  n.children=snapshot(function,depth+1);result.push_back(std::move(n));
 }
 return result;
}
}
std::vector<ConfigNode> parse_config(ConfigLoader loader,const std::string &filename) {
 FileSys::Session session(std::move(loader));
 PTree tree;
 if(!tree.AddFile(filename.c_str()))throw FileSys::FileNotFound(tree.LastError());
 return snapshot(tree.GetGlobalScope());
}
void roundtrip_config(ConfigLoader loader,const std::string &filename,const std::string &text_output,const std::string &binary_output) {
 FileSys::Session session(std::move(loader));
 PTree tree;
 if(!tree.AddFile(filename.c_str()))throw FileSys::FileNotFound(tree.LastError());
 if(!tree.WriteTreeText(text_output.c_str())||!tree.WriteTreeBinary(binary_output.c_str()))
  throw std::runtime_error("Original configuration writer failed");
}
}
