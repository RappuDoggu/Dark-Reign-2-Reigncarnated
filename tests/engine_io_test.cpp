#include "io_prelude.h"
#include "file.h"
#include "packdef.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
void require(bool ok, const char *what) { if (!ok) throw std::runtime_error(what); }
template<class F> void rejects(F f, const char *what) {
  bool caught = false; try { f(); } catch (const std::exception &) { caught = true; }
  require(caught, what);
}
struct Scratch {
  fs::path path;
  Scratch() {
    char pattern[] = "/tmp/dr2-engine-io-XXXXXX";
    auto p = ::mkdtemp(pattern); if (!p) throw std::runtime_error("mkdtemp"); path=p;
  }
  ~Scratch() { std::error_code ec; fs::remove_all(path, ec); }
};
void archive(const char *path) {
  File f; require(f.Open(path, File::READ), "Open real mission pack");
  PackInfo header{};
  require(f.Read(&header, 1, sizeof header) == sizeof header, "Read pack header returns bytes");
  require(header.packId == 0x4b524f4e && header.verId == 1, "NORK pack identity");
  require(header.fileOffset < f.GetSize() && header.fileCount, "Real pack index bounds");
  require(header.fileOffset <= U32(std::numeric_limits<S32>::max()), "Seek range");
  require(f.Seek(File::SET, S32(header.fileOffset)), "Seek pack index");
  U32 entries=0;
  for (; entries < header.fileCount; ++entries) {
    U32 dir=0, offset=0, length=0; U8 nameLength=0; std::array<char,256> name{};
    require(f.Read(&dir,4)==4 && f.Read(&nameLength,1)==1, "Pack entry prefix");
    require(nameLength && f.Read(name.data(),nameLength)==nameLength, "Pack filename");
    require(f.Read(&offset,4)==4 && f.Read(&length,4)==4, "Pack extent");
    require(offset <= f.GetSize() && length <= f.GetSize()-offset, "Pack data bounded");
    U32 resume=f.Tell();
    if (length) {
      void *aligned=nullptr;
      auto mapped=static_cast<U8 *>(f.MapMemory(name.data(),File::MAP_READ,offset,length,&aligned));
      require(f.Seek(File::SET,S32(offset)), "Seek entry");
      std::vector<U8> copy(length);
      require(f.Read(copy.data(),length)==length, "Read entry");
      require(std::memcmp(mapped,copy.data(),length)==0, "Mapped archive bytes match sequential read");
      f.UnmapMemory(aligned);
    }
    require(f.Seek(File::SET,S32(resume)), "Restore index cursor");
  }
  f.Close();
  std::cout << "PASS real archive: " << entries << " entries mapped/read: " << path << '\n';
}
int main(int argc,char **argv) {
  try {
    Scratch scratch;
    FilePath identifier; Dir::PathMake(identifier,"missions","multiplayer");
    require(std::string(identifier.str)=="missions\\multiplayer","Preserve engine path CRC spelling");
    auto folder=scratch.path/"MiSsIoNs";
    require(Dir::MakeFull((folder/"nested").c_str()),"Create mission directory");
    auto path=folder/"TeSt.BIN";
    std::vector<U8> expected(20003);
    for (std::size_t i=0;i<expected.size();++i) expected[i]=U8(i*17);
    File f;
    require(f.Open(path.c_str(),File::READ|File::WRITE|File::CREATE),"Create");
    require(f.Write(expected.data(),expected.size())==expected.size(),"Write");
    require(f.GetSize()==expected.size(),"Size");
    require(f.Seek(File::SET,0),"Rewind");
    std::vector<U8> read(expected.size()+10);
    require(f.Read(read.data(),1,read.size())==expected.size(),"Short read bytes");
    require(std::equal(expected.begin(),expected.end(),read.begin()),"Read contents");
    rejects([&]{f.Read(read.data(),0xffffffff,2);},"Reject product overflow");
    void *aligned=nullptr;
    auto mapped=static_cast<U8 *>(f.MapMemory("mission",File::MAP_READ,123,8197,&aligned));
    require(std::memcmp(mapped,expected.data()+123,8197)==0,"Unaligned mapped range");
    rejects([&]{f.Close();},"Reject close with active mappings");
    f.UnmapMemory(aligned);
    rejects([&]{f.UnmapMemory(aligned);},"Reject double unmap");
    rejects([&]{f.MapMemory("invalid",File::MAP_READ,20000,4);},"Reject out-of-bounds map");
    mapped=static_cast<U8 *>(f.MapMemory("to-end",File::MAP_READ,123,0));
    require(mapped[expected.size()-124]==expected.back(),"Length zero maps to EOF");
    f.UnmapMemory(mapped);
    mapped=static_cast<U8 *>(f.MapMemory("write",File::MAP_WRITE,19,1));
    mapped[0]=91; expected[19]=91; f.UnmapMemory(mapped); f.Close();
    std::string windows=(scratch.path/"missions"/"test.bin").string();
    std::replace(windows.begin(),windows.end(),'/','\\');
    require(f.Open(windows.c_str(),File::READ),"Case/backslash path resolution");
    require(f.GetSize()==expected.size(),"Resolved correct file"); f.Close();
    require(File::Crc(path.c_str())==Crc::Calc(expected.data(),expected.size()),"Original streaming CRC");
    const char nonascii[]={char(0x80),char(0xff),0};
    require(Crc::CalcStr(nonascii)==Crc::CalcLower(nonascii,2),"Unsigned CRC table indexing");
    require(f.Open(path.c_str(),File::APPEND),"Append open");
    require(f.Seek(File::SET,0),"Append seek"); require(f.WriteU16(0xabcd),"Append little endian"); f.Close();
    require(f.Open(path.c_str(),File::READ),"Read append");
    require(f.GetSize()==expected.size()+2,"Append ignores current cursor"); f.Close();
    require(!f.Open((folder/"missing").c_str(),File::READ),"Missing input fails");
    require(!f.Open("C:\\DR2\\base.zwp",File::READ),"Unmapped Windows drive fails");
    Dir::Find find;
    require(Dir::FindFirst(find,folder.c_str(),"*.bin"),"Case-insensitive wildcard");
    require(std::string(find.finddata.name)=="TeSt.BIN","Original case preserved");
    require(!Dir::FindNext(find),"Enumeration exhausted"); Dir::FindClose(find); Dir::FindClose(find);
    require(Dir::FindFirst(find,folder.c_str(),"*.*",File::SUBDIR),"DOS extensionless directory wildcard");
    require(std::string(find.finddata.name)=="nested","Directory attributes"); Dir::FindClose(find);
    require(!File::Copy(path.c_str(),path.c_str()),"Copy failure reported");
    auto copy=folder/"copy.bin";
    require(File::Copy(path.c_str(),copy.c_str()),"Copy");
    require(File::Crc(copy.c_str())==File::Crc(path.c_str()),"Copy bytes");
    require(!File::Copy(path.c_str(),copy.c_str()),"No implicit overwrite");
    require(File::Unlink(copy.c_str()),"Unlink");
    std::cout << "PASS native File/Dir: I/O, mappings, paths, enumeration, CRC, failure paths\n";
    for(int i=1;i<argc;++i) archive(argv[i]);
  } catch(const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
