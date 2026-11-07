#include "reign/babel_support.hpp"
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <stdexcept>
#include <vector>
namespace reign::babel_detail {
namespace {
constexpr std::size_t limit=16384;
std::size_t length(const CH *s) {
 if(!s)throw std::invalid_argument("Null Babel text");
 for(std::size_t n=0;n<limit;++n)if(!s[n])return n;
 throw std::length_error("Babel text exceeds buffer limit");
}
struct Spec {std::string ascii;char type;bool left=false;unsigned width=0;int precision=-1;};
Spec spec(const CH *s) {
 Spec r;const auto n=length(s);
 if(n<2||n>510||s[0]!='%')throw std::invalid_argument("Invalid Babel format");
 for(std::size_t i=0;i<n;++i){if(s[i]>127)throw std::invalid_argument("Non-ASCII Babel specifier");r.ascii+=char(s[i]);}
 std::size_t at=1;
 while(at<n&&std::strchr("-+ #0",r.ascii[at])) {r.left|=r.ascii[at]=='-';++at;}
 auto number=[&](){unsigned result=0;while(at<n&&r.ascii[at]>='0'&&r.ascii[at]<='9'){
  result=result*10+unsigned(r.ascii[at++]-'0');if(result>500)throw std::length_error("Babel field too large");}return result;};
 r.width=number();
 if(at<n&&r.ascii[at]=='.'){++at;r.precision=int(number());}
 if(at!=n-1||!std::strchr("cCdiouxXeEfgGsS",r.ascii[at]))throw std::invalid_argument("Unsupported Babel format specifier");
 r.type=r.ascii[at];return r;
}
template<class T> void numeric(CH *out,U32 capacity,const Spec &f,T value) {
 char text[512];int count=std::snprintf(text,sizeof(text),f.ascii.c_str(),value);
 if(count<0||count>=int(sizeof(text)))throw std::length_error("Babel numeric result too large");
 if(U32(count)>=capacity)throw std::length_error("Babel output too small");
 for(int i=0;i<count;++i)out[i]=static_cast<unsigned char>(text[i]);out[count]=0;
}
}
CH *find(const CH *s,CH ch){for(;*s;++s)if(*s==ch)return const_cast<CH*>(s);return nullptr;}
U32 index(const CH *s){U32 value=0;while(*s>='0'&&*s<='9'){if(value>1000)throw std::invalid_argument("Babel argument index too large");value=value*10+(*s++-'0');}return value;}
void validate(const CH *s,U32 params) {
 const auto n=length(s);if(params>128)throw std::invalid_argument("Too many Babel arguments");std::set<U32> seen;
 for(std::size_t at=0;at<n;++at)if(s[at]=='{'){
  if(at+1<n&&s[at+1]=='{'){++at;continue;}
  ++at;const auto start=at;while(at<n&&s[at]>='0'&&s[at]<='9')++at;
  if(at==start||at==n||s[at]!=':')throw std::invalid_argument("Malformed Babel argument");
  U32 id=index(s+start);if(!id||id>params||!seen.insert(id).second)throw std::invalid_argument("Duplicate or invalid Babel argument");
  std::vector<CH> f{'%'};++at;while(at<n&&s[at]!='}')f.push_back(s[at++]);
  if(at==n)throw std::invalid_argument("Unclosed Babel argument");f.push_back(0);(void)spec(f.data());
 }
 U32 next=1;for(U32 id:seen)if(id!=next++)throw std::invalid_argument("Missing Babel argument index");
}
void copy(CH *out,U32 capacity,const CH *s,std::size_t n){
 if(!out||!s||n>=capacity)throw std::length_error("Babel output too small");
 std::memmove(out,s,n*sizeof(CH));out[n]=0;
}
void copy(CH *out,U32 capacity,const CH *s){copy(out,capacity,s,length(s));}
void unknown(CH *out,U32 capacity,const char *key){
 std::string text="Unknown[";text+=key;text+=']';
 if(text.size()>=capacity)throw std::length_error("Unknown Babel key too long");
 Utils::Ansi2Unicode(out,capacity,text.c_str());
}
void format(CH *out,U32 capacity,const CH *f,int value){
 auto p=spec(f);
 if(p.type=='c'||p.type=='C'){
  if(value<=0||value>65535)throw std::invalid_argument("Invalid UTF16 character");
  CH text[]={CH(value),0};std::vector<CH> sf{'%'};if(p.left)sf.push_back('-');
  for(char c:std::to_string(p.width))sf.push_back(CH(c));sf.push_back('s');sf.push_back(0);format(out,capacity,sf.data(),text);
 }else if(std::strchr("ouxX",p.type))numeric(out,capacity,p,static_cast<unsigned>(value));
 else if(p.type=='d'||p.type=='i')numeric(out,capacity,p,value);
 else throw std::invalid_argument("Babel integer type mismatch");
}
void format(CH *out,U32 capacity,const CH *f,double value){auto p=spec(f);if(!std::strchr("eEfgG",p.type))throw std::invalid_argument("Babel floating type mismatch");numeric(out,capacity,p,value);}
void format(CH *out,U32 capacity,const CH *f,const CH *value){
 auto p=spec(f);if(p.type!='s'&&p.type!='S')throw std::invalid_argument("Babel string type mismatch");
 auto n=length(value);if(p.precision>=0&&n>std::size_t(p.precision))n=p.precision;
 const auto total=std::max(n,std::size_t(p.width));if(total>=capacity)throw std::length_error("Babel string result too large");
 std::size_t at=0;if(!p.left)while(at<total-n)out[at++]=' ';
 for(std::size_t i=0;i<n;++i)out[at++]=value[i];while(at<total)out[at++]=' ';out[at]=0;
}
void format(CH *,U32,const CH *,int *){throw std::invalid_argument("Babel count writes unsupported");}
void format(CH *,U32,const CH *,void *){throw std::invalid_argument("Babel pointer formatting unsupported");}
}
