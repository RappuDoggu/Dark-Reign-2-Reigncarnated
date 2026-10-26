#pragma once
#include "catalog_types.hpp"
#include <vector>
namespace reign::mission_fs {
using FilePath=StrBuf<270>;
namespace File {namespace Attrib {constexpr unsigned SUBDIR=16;}bool Exists(const char *);}
namespace Dir {
struct Find {struct Entry{unsigned attrib=0;char name[128]{};};Entry finddata;std::vector<Entry> entries;std::size_t index=0;};
bool FindFirst(Find &,const char *,const char *,unsigned flags=0);
bool FindNext(Find &);void FindClose(Find &);
void PathMake(FilePath &,const char *,const char *);
void PathMake(FilePath &,const char *,const char *,const char *,const char *);
}
}
