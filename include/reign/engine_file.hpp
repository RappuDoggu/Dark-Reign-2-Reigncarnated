#pragma once
#include "reign/legacy_types.hpp"
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <vector>
#include <stdexcept>
// Narrow preserved File/DataFile contract used by original PTree/BlockFile.
class File {
 std::fstream stream_;
public:
 enum { READ=1, WRITE=2, CREATE=8, SET=0, CUR=1, END=2 };
 bool Open(const char *, U32);
 void Close();
 bool IsOpen() const { return stream_.is_open(); }
 bool Seek(U32, U32);
 U32 Write(const void *, U32);
 void WriteU8(U8 c) { Write(&c,1); }
 static bool Unlink(const char *);
};
namespace FileSys {
class FileNotFound:public std::runtime_error { public:using std::runtime_error::runtime_error; };
class DataFile {
 std::vector<U8> bytes_;
 U32 position_=0;
public:
 explicit DataFile(std::vector<U8> bytes):bytes_(std::move(bytes)){}
 U32 Size() const { return static_cast<U32>(bytes_.size()); }
 void *GetMemoryPtr() { return bytes_.data(); }
 U32 Read(void *, U32);
 bool Seek(U32);
};
using Loader = std::function<std::vector<U8>(const std::string &)>;
// Thread-scoped loader. Owns open handles even if an original fatal error throws.
class Session {
 struct Impl; std::unique_ptr<Impl> impl_;
 friend DataFile *Open(const char *);
 friend void Close(DataFile *);
public:
 explicit Session(Loader loader);
 ~Session();
 Session(const Session &)=delete;
 Session &operator=(const Session &)=delete;
};
DataFile *Open(const char *);
void Close(DataFile *);
}
