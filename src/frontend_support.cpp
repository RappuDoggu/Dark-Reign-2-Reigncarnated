#include "reign/frontend_support.hpp"
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
namespace Utils {
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
