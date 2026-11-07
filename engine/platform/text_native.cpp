#include "resource_prelude.h"
#include "text_native.h"
#include <string>
#include <vector>
#include <climits>
namespace {
constexpr CH extended[32]={0x20ac,0x81,0x201a,0x192,0x201e,0x2026,0x2020,0x2021,0x2c6,0x2030,0x160,0x2039,0x152,0x8d,0x17d,0x8f,0x90,0x2018,0x2019,0x201c,0x201d,0x2022,0x2013,0x2014,0x2dc,0x2122,0x161,0x203a,0x153,0x9d,0x17e,0x178};
CH Decode(unsigned char c){return c>=0x80&&c<0xa0?extended[c-0x80]:c;}
char Encode(CH c){for(unsigned i=0;i<32;++i)if(extended[i]==c)return char(i+0x80);return c<256?char(c):'?';}
CH Lower(CH c){return c>='A'&&c<='Z'?CH(c+'a'-'A'):c;}
using Text=std::basic_string<CH>;
Text Ansi(const char *s){Text result;if(!s)s="(null)";while(*s)result+=Decode(static_cast<unsigned char>(*s++));return result;}
template<class T> Text Number(const std::string &format,T value){
 const int count=std::snprintf(nullptr,0,format.c_str(),value);
 if(count<0||count>65536)throw std::length_error("Engine numeric format length");
 std::vector<char> buffer(std::size_t(count)+1);
 std::snprintf(buffer.data(),buffer.size(),format.c_str(),value);return Ansi(buffer.data());
}
}
namespace Utils {
U32 Strlen(const CH *s){U32 n=0;while(s[n])++n;return n;}
CH *Strcpy(CH *d,const CH *s){CH *start=d;do{*d++=*s;}while(*s++);return start;}
CH *Strncpy(CH *d,const CH *s,U32 count){U32 i=0;for(;i<count&&s[i];++i)d[i]=s[i];for(;i<count;++i)d[i]=0;return d;}
CH *Strmcpy(CH *d,const CH *s,U32 count){if(count){U32 i=0;for(;i+1<count&&s[i];++i)d[i]=s[i];d[i]=0;}return d;}
CH *Strchr(const CH *s,CH c){do{if(*s==c)return const_cast<CH *>(s);}while(*s++);return nullptr;}
CH *Strdup(const CH *s){auto *d=new CH[Strlen(s)+1];return Strcpy(d,s);}
CH *Strndup(const CH *s,U32 count){const U32 n=std::min(Strlen(s),count);auto *d=new CH[n+1];std::copy(s,s+n,d);d[n]=0;return d;}
S32 Strcmp(const CH *a,const CH *b){while(*a&&*a==*b){++a;++b;}return int(*a)-int(*b);}
S32 Stricmp(const CH *a,const CH *b){while(*a&&Lower(*a)==Lower(*b)){++a;++b;}return int(Lower(*a))-int(Lower(*b));}
S32 AtoI(const CH *s){std::string ascii;while(*s&&*s<128)ascii+=char(*s++);return std::atoi(ascii.c_str());}
int Ansi2Unicode(CH *out,U32 capacity,const char *input,U32 length){if(!capacity)return 0;const U32 n=std::min(length,capacity-1);for(U32 i=0;i<n;++i)out[i]=Decode(static_cast<unsigned char>(input[i]));out[n]=0;return int(n+1);}
int Ansi2Unicode(CH *out,U32 capacity,const char *input){return Ansi2Unicode(out,capacity,input,Strlen(input));}
const CH *Ansi2Unicode(const char *input){static thread_local CH out[256];Ansi2Unicode(out,256,input);return out;}
int Unicode2Ansi(char *out,U32 capacity,const CH *input){if(!capacity)return 0;U32 n=0;for(;n+1<capacity&&input[n];++n)out[n]=Encode(input[n]);out[n]=0;return int(n+1);}
const char *Unicode2Ansi(const CH *input){static thread_local char out[256];Unicode2Ansi(out,256,input);return out;}
S32 CDECL Sprintf(CH *out,S32 capacity,const CH *format,...){va_list args;va_start(args,format);S32 n;try{n=NativeEngine::VFormat16(out,capacity,format,args);}catch(...){va_end(args);throw;}va_end(args);return n;}
}
namespace NativeEngine {
S32 VFormat16(CH *out,S32 capacity,const CH *format,va_list args){
 if(capacity<=0)return -1;
 out[0]=0;Text result;
 for(const CH *p=format;*p;++p){
  if(*p!='%'){result+=*p;continue;}
  ++p;if(*p=='%'){result+='%';continue;}
  std::string flags;while(*p&&std::strchr("-+ #0",char(*p)))flags+=char(*p++);
  int width=0,precision=-1;
  if(*p=='*'){width=va_arg(args,int);++p;if(width==INT_MIN)throw std::length_error("Format width");if(width<0){flags+='-';width=-width;}}
  else while(*p>='0'&&*p<='9'){if(width>6553)throw std::length_error("Format width");width=width*10+(*p++-'0');}
  if(*p=='.'){++p;precision=0;if(*p=='*'){precision=va_arg(args,int);++p;}else while(*p>='0'&&*p<='9'){if(precision>6553)throw std::length_error("Format precision");precision=precision*10+(*p++-'0');}}
  if(width>65536||precision>65536)throw std::length_error("Format field size");
  std::string length;if(*p=='l'||*p=='h'){length+=char(*p++);if(*p==length[0])length+=char(*p++);}
  if(!*p||*p>127)throw std::invalid_argument("Invalid UTF-16 format");
  const char spec=char(*p);Text field;
  if(spec=='s'||spec=='S'){
   if(spec=='S'||length=="h")field=Ansi(va_arg(args,const char *));
   else {const CH *s=va_arg(args,const CH *);field=s?Text(s):Ansi("(null)");}
   if(precision>=0&&field.size()>std::size_t(precision))field.resize(precision);
  } else if(spec=='c'||spec=='C')field+=CH(va_arg(args,int));
  else {
   std::string f="%"+flags+(width?std::to_string(width):"")+(precision>=0?"."+std::to_string(precision):"");
   if(spec=='d'||spec=='i') {if(length=="ll")field=Number(f+"ll"+spec,va_arg(args,long long));else field=Number(f+spec,va_arg(args,int));}
   else if(std::strchr("uoxX",spec)){if(length=="ll")field=Number(f+"ll"+spec,va_arg(args,unsigned long long));else field=Number(f+spec,va_arg(args,unsigned int));}
   else if(std::strchr("fFeEgGaA",spec))field=Number(f+spec,va_arg(args,double));
   else if(spec=='p')field=Number(f+spec,va_arg(args,void *));
   else throw std::invalid_argument("Unsupported engine format conversion");
   width=0; // Numeric padding was handled by snprintf.
  }
  if(width>int(field.size())){Text padding(std::size_t(width)-field.size(),' ');field=flags.find('-')!=std::string::npos?field+padding:padding+field;}
  if(result.size()+field.size()>1048576)throw std::length_error("Engine formatted output too large");
  result+=field;
 }
 const std::size_t count=std::min(result.size(),std::size_t(capacity-1));std::copy_n(result.data(),count,out);out[count]=0;
 return result.size()>=std::size_t(capacity)?-1:S32(result.size());
}
}
namespace Utils {
void CharDecode(U8 *out,const char *input,U32 length,U32 capacity){
 if((length&1)||length/2>capacity)throw std::length_error("Invalid hex data size");
 auto nibble=[](unsigned char c)->U8{if(c>='0'&&c<='9')return U8(c-'0');if(c>='a'&&c<='f')return U8(c-'a'+10);if(c>='A'&&c<='F')return U8(c-'A'+10);throw std::invalid_argument("Invalid hex data digit");};
 // DR2 encodes the low nibble first; this is not conventional hex byte order.
 for(U32 i=0;i<length;i+=2)out[i/2]=U8(nibble(input[i])|(nibble(input[i+1])<<4));
}
void CharEncode(char *out,const U8 *input,U32 length,U32 capacity){
 if(std::uint64_t(length)*2+1>capacity)throw std::length_error("Hex output capacity");
 constexpr char digits[]="0123456789ABCDEF";
 for(U32 i=0;i<length;++i){out[i*2]=digits[input[i]&15];out[i*2+1]=digits[input[i]>>4];}out[length*2]=0;
}
char *FindExt(const char *path){const char *p=path+std::strlen(path);while(p!=path){if(*p=='.')return const_cast<char *>(p);--p;}return nullptr;}
char *StrFmtHex(char *output,U32 width,U32 value,U32 flags){
 static thread_local char buffer[256];if(!output){if(width>=sizeof(buffer))throw std::length_error("Hex field width");output=buffer;}
 if(width>U32(INT_MAX))throw std::length_error("Hex field width");
 output[width]=0;int pos=int(width)-1;bool end=false;
 if((flags&STRFMT_HEXSUFFIX)&&pos>=0)output[pos--]='h';
 constexpr char digits[]="0123456789ABCDEF";
 while(pos>=0){if(end){output[pos--]=' ';continue;}output[pos--]=digits[value&15];value>>=4;end=(flags&STRFMT_HEXPREFIX)&&value==0;if(end&&pos>=2){output[pos--]='x';output[pos--]='0';}}
 return output;
}
}

namespace Utils { S32 Strncmp(const char *a, const char *b, U32 count) { return std::strncmp(a,b,count); } }

namespace Utils {
char *ChopPath(const char *path) {
 const char *name=path;
 for(const char *p=path;*p;++p)if(*p=='/'||*p=='\\')name=p+1;
 return const_cast<char *>(name);
}
}

namespace Utils {
CH *ItoA(int value, CH *buffer, int radix) {
 if (!buffer || radix<2 || radix>36) throw std::invalid_argument("Invalid integer conversion");
 const bool negative=radix==10 && value<0;
 U32 magnitude=negative?U32(0)-U32(value):U32(value);
 CH digits[32]; unsigned count=0;
 do { const U32 digit=magnitude%U32(radix); digits[count++]=CH(digit<10?'0'+digit:'a'+digit-10); magnitude/=U32(radix); } while(magnitude);
 CH *out=buffer; if(negative)*out++='-';
 while(count)*out++=digits[--count]; *out=0; return buffer;
}
}

#include <cmath>
namespace {thread_local char fmtBuf[256];}
namespace Utils {
  char *StrFmtDec(char *s, U32 width, S32 value, U32 flags)
  {
    std::int64_t number=value;
    Bool end    = FALSE;
    Bool neg    = FALSE;
    S32  comma  = 3;
	char *start = s;
	char* p = nullptr;
  
    if (number < 0)
    {
      neg = TRUE;
      number = -number;
    }

    // Use default buffer if none specified
    if (s == NULL)
    {
      ASSERT(width < sizeof(fmtBuf));
      start = fmtBuf;
    }

    p = start + width - 1;
    *(p + 1) = '\0';

    // Format the number
    while (p >= start)
    {
      if (!end)
      {
        *p-- = char((number % 10) + '0');
        number /= 10;
        end = number == 0;

        if (end)
        {
          // Prepend the negative sign
          if (neg && (p > start))
          {
            *p-- = '-';
          }
        }
        else
        {
          // Insert a comma
          if ((flags & STRFMT_COMMA) && (p > start) && (--comma == 0))
          {
            *p-- = ',';
            comma = 3;
          }
        }
      }
      else
      {
        *p-- = ' ';
      }
    }

    return start;
  }
  char *StrFmtFloat(char *s, U32 width, U32 decpt, F32 value, U32)
  {
    ASSERT(width > decpt);

    Bool end    = FALSE;
    Bool neg = FALSE;
    char *start = s, *p;

    // Use default buffer if not specified
    if (s == NULL)
    {
      ASSERT(width < sizeof(fmtBuf));
      start = fmtBuf;
    }

    p = start + width - 1;
    *(p + 1) = '\0';

    F64 fFrac, fInt;
    S32 iFrac, iInt;

    fFrac = modf(value, &fInt);

    // Format fractional part
    if (decpt)
    {
      U32 n = decpt;

      while (n--)
      {
        fFrac *= 10.0;
      }

      iFrac = Utils::FtoL(F32(fFrac));

      if (iFrac < 0)
      {
        iFrac = -iFrac;
      }

      while (decpt--)
      {
        *p-- = char((iFrac % 10) + '0');
        iFrac /= 10;
      }
      *p-- = '.';
    }

    // Format integer part
    iInt = Utils::FtoL(F32(fInt));

    if (iInt < 0)
    { 
      neg = TRUE;
      iInt = -iInt;
    }

    while (p >= start)
    {
      if (!end)
      {
        *p-- = char((iInt % 10) + '0');
        iInt /= 10;
        end = iInt == 0;

        if (end && neg && p > start)
        {
          *p-- = '-';
        }
      }
      else
      {
        *p-- = ' ';
      }
    }

    return start;
  }
}
