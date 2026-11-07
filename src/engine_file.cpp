#include "reign/engine_file.hpp"
#include <cstring>
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <unordered_set>
namespace FileSys {
namespace { thread_local Session *active=nullptr; }
struct Session::Impl {
 Loader loader;
 Session *previous;
 std::vector<std::unique_ptr<DataFile>> handles;
 Impl(Loader f):loader(std::move(f)),previous(active){}
};
Session::Session(Loader loader):impl_(std::make_unique<Impl>(std::move(loader))) { active=this; }
Session::~Session() { active=impl_->previous; }
DataFile *Open(const char *name) {
 if (!active) throw std::runtime_error("Engine file session is not configured");
 try {
  if(active->impl_->handles.size()>=64)throw std::runtime_error("Config include depth exceeds 64 open files");
  auto bytes=active->impl_->loader(name);
  // Validate container extents before the legacy BlockFile obtains raw pointers.
  if(bytes.size()>=4 && std::memcmp(bytes.data(),"DOCB",4)==0) {
   auto word=[&](std::size_t p)->std::uint32_t {
    if(p>bytes.size() || bytes.size()-p<4)throw std::runtime_error("Truncated DOCB word");
    return std::uint32_t(bytes[p])|(std::uint32_t(bytes[p+1])<<8)|(std::uint32_t(bytes[p+2])<<16)|(std::uint32_t(bytes[p+3])<<24);
   };
   if(bytes.size()<44 || word(4)!=1)throw std::runtime_error("Unsupported/truncated DOCB header");
   const auto count=word(8);if(count>(bytes.size()-44)/8)throw std::runtime_error("Invalid DOCB block count");
   std::size_t at=44;std::unordered_set<std::uint32_t> keys;
   for(std::uint32_t n=0;n<count;++n){
    auto key=word(at),length=word(at+4);at+=8;
    if(length>bytes.size()-at || !keys.insert(key).second)throw std::runtime_error("Invalid DOCB block range/key");
    at+=length;
   }
  }
  if(bytes.size()>std::numeric_limits<U32>::max()) throw std::runtime_error("Original DataFile size overflow");
  auto file=std::make_unique<DataFile>(std::move(bytes));
  auto *p=file.get();active->impl_->handles.push_back(std::move(file));return p;
 } catch(const FileNotFound &) { return nullptr; }
}
void Close(DataFile *file) {
 if(!file) return;
 if(!active) throw std::runtime_error("Engine file close without session");
 auto &v=active->impl_->handles;
 auto it=std::find_if(v.begin(),v.end(),[&](auto &p){return p.get()==file;});
 if(it==v.end()) throw std::runtime_error("Foreign engine file handle");
 v.erase(it);
}
U32 DataFile::Read(void *out,U32 count) {
 const U32 available=Size()-position_;count=std::min(count,available);
 if(count)std::memcpy(out,bytes_.data()+position_,count);position_+=count;return count;
}
bool DataFile::Seek(U32 position) { if(position>Size())return false;position_=position;return true; }
}
bool File::Open(const char *name,U32 mode) {
 Close();stream_.clear();auto flags=std::ios::binary;
 if(mode&WRITE)flags|=std::ios::out;else flags|=std::ios::in;
 if(mode&CREATE)flags|=std::ios::trunc;else if(mode&WRITE)flags|=std::ios::in;
 stream_.open(name,flags);return stream_.is_open();
}
void File::Close() { if(stream_.is_open())stream_.close(); }
bool File::Seek(U32 mode,U32 position) {
 auto origin=mode==SET?std::ios::beg:mode==CUR?std::ios::cur:std::ios::end;
 stream_.seekp(position,origin);return static_cast<bool>(stream_);
}
U32 File::Write(const void *data,U32 count) { if(count)stream_.write(static_cast<const char *>(data),count);return stream_?count:0; }
bool File::Unlink(const char *name) { return std::filesystem::remove(name); }
