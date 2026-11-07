#include "resource_prelude.h"
#include "type_ident_registry.h"
#include <map>
#include <mutex>
namespace NativeEngine {
namespace {
std::mutex mutex;
std::map<U32,GameIdent> pending;
U32 nextToken=1;
}
U32 StoreTypeIdent(const char *name) {
 if(!name || !*name) throw std::invalid_argument("Empty object type name");
 std::lock_guard<std::mutex> lock(mutex);
 // Never recycle tokens: stale copied reapers must not resolve to a new type.
 if(nextToken==0) throw std::overflow_error("Object type token space exhausted");
 const U32 token=nextToken;
 pending.emplace(token,GameIdent(name));
 ++nextToken;
 return token;
}
GameIdent TakeTypeIdent(U32 token) {
 std::lock_guard<std::mutex> lock(mutex);
 auto found=pending.find(token);
 if(found==pending.end()) throw std::logic_error("Invalid or consumed object type token");
 GameIdent name=found->second;
 pending.erase(found);
 return name;
}
void ClearTypeIdents() { std::lock_guard<std::mutex> lock(mutex); pending.clear(); }
std::size_t PendingTypeIdents() { std::lock_guard<std::mutex> lock(mutex); return pending.size(); }
}
namespace StdLoad {
U32 GetIdentAsU32(const char *ident) { return NativeEngine::StoreTypeIdent(ident); }
}
