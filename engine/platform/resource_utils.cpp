#include "resource_prelude.h"
#include "file.h"
#include <cfenv>
#include <strings.h>
namespace Utils {
U32 Strlen(const char *s) { return static_cast<U32>(std::strlen(s)); }
S32 Strcmp(const char *a,const char *b) { return std::strcmp(a,b); }
S32 Stricmp(const char *a,const char *b) {
  for(;;++a,++b) {
    unsigned char x=*a,y=*b;
    if(x>='A'&&x<='Z') x+='a'-'A';
    if(y>='A'&&y<='Z') y+='a'-'A';
    if(x!=y || !x) return int(x)-int(y);
  }
}
char *Strcpy(char *d,const char *s) { return std::strcpy(d,s); }
char *Strncpy(char *d,const char *s,U32 n) { return std::strncpy(d,s,n); }
char *Strcat(char *d,const char *s) { return std::strcat(d,s); }
char *Strchr(const char *s,char c) { return const_cast<char *>(std::strchr(s,c)); }
char *Strrchr(const char *s,char c) { return const_cast<char *>(std::strrchr(s,c)); }
char *Strstr(const char *s,const char *p) { return const_cast<char *>(std::strstr(s,p)); }
char *Strdup(const char *s) { auto n=std::strlen(s)+1; auto p=new char[n]; std::memcpy(p,s,n); return p; }
S32 AtoI(const char *s) { return std::atoi(s); }
F32 AtoF(const char *s) { return std::strtof(s,nullptr); }
S32 Sprintf(char *out,S32 size,const char *format,...) {
  if(size<=0) return -1;
  va_list ap; va_start(ap,format); int n=std::vsnprintf(out,size,format,ap); va_end(ap); return n;
}
void MakePath(char *out,U32 size,const char *dir,const char *name,const char *ext) {
  FilePath p; Dir::PathMake(p,"",dir,name,ext);
  if(std::strlen(p.str)>=size) throw std::length_error("MakePath output capacity");
  Strmcpy(out,p.str,size);
}
}
