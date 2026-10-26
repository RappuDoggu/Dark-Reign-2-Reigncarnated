#include "reign/frontend_support.hpp"
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
namespace {thread_local reign::AnsiEncoding ansi_encoding=reign::AnsiEncoding::ascii;}
namespace reign {
AnsiBinding::AnsiBinding(AnsiEncoding value):previous_(ansi_encoding){ansi_encoding=value;}
AnsiBinding::~AnsiBinding(){ansi_encoding=previous_;}
}
namespace Utils {
int Ansi2Unicode(CH *out,U32 size,const char *input){
 if(!out||!size||!input||size>U32(std::numeric_limits<int>::max()))throw std::invalid_argument("Invalid ANSI conversion buffer");
 static constexpr CH cp1252[32]={0x20ac,0x81,0x201a,0x192,0x201e,0x2026,0x2020,0x2021,0x2c6,0x2030,0x160,0x2039,0x152,0x8d,0x17d,0x8f,0x90,0x2018,0x2019,0x201c,0x201d,0x2022,0x2013,0x2014,0x2dc,0x2122,0x161,0x203a,0x153,0x9d,0x17e,0x178};
 U32 n=0;while(n<size-1&&input[n]){auto c=static_cast<unsigned char>(input[n]);
  if(c>=128&&ansi_encoding==reign::AnsiEncoding::ascii)throw std::runtime_error("Installation ANSI code page not selected");
  ++n;
 }
 // Validate before modifying destination; preserve count including terminating NUL.
 for(U32 i=0;i<n;++i){auto c=static_cast<unsigned char>(input[i]);out[i]=(c>=128&&c<160)?cp1252[c-128]:CH(c);}
 out[n]=0;return int(n+1);
}
char *StrFmtHex(char *out,U32 width,U32 value,U32 flags){
 thread_local char buffer[35];
 if(!width||(!out&&width>=sizeof(buffer)))throw std::invalid_argument("Invalid hexadecimal field width");
 if(!out)out=buffer;
 static constexpr char digits[]="0123456789ABCDEF";
 std::int64_t p=std::int64_t(width)-1;out[width]=0;bool end=false;
 if(flags&4)out[p--]='h';
 while(p>=0){if(!end){out[p--]=digits[value&15];value>>=4;end=(flags&2)&&(value==0);
   if(end&&p>=2){out[p--]='x';out[p--]='0';}
  }else out[p--]=' ';
 }
 return out;
}
U32 Strlen(const CH *s){
 if(!s)throw std::invalid_argument("Null UTF-16 string");
 std::size_t n=0;
 while(s[n]){if(n==std::numeric_limits<U32>::max())throw std::length_error("UTF-16 string too long");++n;}
 return static_cast<U32>(n);
}
CH *Strmcpy(CH *out,const CH *in,U32 capacity){
 if(!out||!in||!capacity)throw std::invalid_argument("Invalid UTF-16 copy buffer");
 // Windows wcsncpy semantics: count code units, pad short strings, force NUL.
 U32 n=0;while(n<capacity&&in[n]){out[n]=in[n];++n;}
 while(n<capacity)out[n++]=0;
 out[capacity-1]=0;return out;
}
CH *Strdup(const CH *s){
 const U32 length=Strlen(s);
 if(length==std::numeric_limits<U32>::max())throw std::length_error("UTF-16 duplication overflow");
 auto *copy=new CH[std::size_t(length)+1];
 std::memcpy(copy,s,(std::size_t(length)+1)*sizeof(CH));return copy;
}
}
namespace Debug::Memory::Aligning {
void *AligningAlloc(U32 size,U32 alignment){
 if(alignment&&(alignment&(alignment-1)))throw std::invalid_argument("Alignment must be a power of two");
 if(!size)return nullptr;
 void *p=nullptr;
 if(!alignment)p=std::malloc(size);
 else if(posix_memalign(&p,alignment<sizeof(void*)?sizeof(void*):alignment,size)!=0)p=nullptr;
 if(!p)throw std::bad_alloc();return p;
}
void AligningFree(void *p){std::free(p);}
}

namespace reign {
namespace frontend_submission {thread_local std::function<void(std::function<void()>)> enqueue;}
bool defer_frontend_submission(std::function<void()> command){
 if(!command)throw std::invalid_argument("Missing frontend submission");
 if(!frontend_submission::enqueue)return false;
 frontend_submission::enqueue(std::move(command));return true;
}
}
