// Native implementation of DR2's original File/Dir interface.
// Works on Linux and Android; the original resource-stream layer can use this
// without a Windows ABI, emulated handles, or narrowing a pointer to 32 bits.
#include "io_prelude.h"
#include "file.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdarg>
#include <filesystem>
#include <fcntl.h>
#include <fnmatch.h>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
namespace {
char lower(char c) { return c >= 'A' && c <= 'Z' ? char(c + ('a' - 'A')) : c; }
std::string fold(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), lower); return s;
}
// Windows data uses backslashes and inconsistent ASCII case. Exact matches win;
// an ambiguous case-insensitive lookup fails rather than choosing arbitrary data.
fs::path nativePath(const char *input) {
  if (!input || !*input) throw std::invalid_argument("Empty engine path");
  std::string s(input);
  std::replace(s.begin(), s.end(), '\\', '/');
  if (s.size() > 1 && s[1] == ':')
    throw std::invalid_argument("Windows drive paths require an explicit data root");
  fs::path result;
  for (const auto &part : fs::path(s)) {
    auto candidate = result / part;
    std::error_code ec;
    if (part == "/" || part == "." || part == ".." || fs::exists(candidate, ec)) {
      result = candidate; continue;
    }
    fs::path match;
    auto parent = result.empty() ? fs::path(".") : result;
    fs::directory_iterator it(parent, ec), end;
    for (; !ec && it != end; it.increment(ec)) {
      if (fold(it->path().filename().string()) == fold(part.string())) {
        if (!match.empty()) throw std::runtime_error("Ambiguous engine path: " + s);
        match = it->path().filename();
      }
    }
    result /= match.empty() ? part : match;
  }
  return result;
}
std::string joined(const char *dir, const char *name) {
  return std::string(dir && *dir ? dir : ".") + "/" + name;
}
void assign(FilePath &out, const fs::path &value) {
  auto s = value.string();
  if (s.size() >= out.GetSize()) throw std::length_error("Engine path exceeds FilePath capacity");
  out = s.c_str();
}
U32 checkedSize(std::uint64_t n) {
  if (n > std::numeric_limits<U32>::max()) throw std::length_error("File exceeds original 32-bit engine size");
  return static_cast<U32>(n);
}
[[noreturn]] void failure(const char *operation) {
  throw std::runtime_error(std::string(operation) + ": " + std::strerror(errno));
}
U32 attributes(const fs::path &p) {
  struct stat st{};
  if (::stat(p.c_str(), &st)) failure("stat");
  U32 flags = S_ISDIR(st.st_mode) ? File::SUBDIR : File::ARCHIVE;
  if (!(st.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH))) flags |= File::READONLY;
  auto name = p.filename().string();
  if (!name.empty() && name[0] == '.') flags |= File::HIDDEN;
  return flags;
}
}

struct NativeFileState {
  int descriptor = -1;
  U32 mode = 0;
  struct Mapping { void *base; void *data; std::size_t length; };
  std::vector<Mapping> mappings;
  ~NativeFileState() {
    for (const auto &m : mappings) ::munmap(m.base, m.length);
    if (descriptor >= 0) ::close(descriptor);
  }
};
struct NativeFindState {
  std::vector<fs::path> paths;
  std::size_t index = 0;
};

// Original StrBuf uses this API; retain its count-includes-terminator contract.
namespace Utils {
char *Strmcpy(char *dest, const char *src, U32 count) {
  if (!dest || !src || !count) throw std::invalid_argument("Strmcpy arguments");
  std::size_t length=0;while(length+1<count&&src[length])++length;
  std::memmove(dest,src,length);std::memset(dest+length,0,count-length);return dest;
}
}

File::File() : native(new NativeFileState) {}
File::~File() { delete native; }
Bool File::Open(const char *dir, const char *name, U32 flags) {
  return Open(joined(dir, name).c_str(), flags);
}
Bool File::Open(const char *path, U32 flags) {
  if (IsOpen()) throw std::logic_error("File already open");
  if (!(flags & (READ | WRITE | APPEND))) return FALSE;
  int access = (flags & APPEND) || ((flags & READ) && (flags & WRITE)) ? O_RDWR :
    (flags & WRITE) ? O_WRONLY : O_RDONLY;
  if (flags & CREATE) access |= O_CREAT | O_TRUNC;
  if (flags & APPEND) access |= O_CREAT | O_APPEND;
  try {
    auto p = nativePath(path);
    native->descriptor = ::open(p.c_str(), access | O_CLOEXEC, 0666);
  } catch (const std::exception &) { return FALSE; }
  native->mode = flags;
  return IsOpen();
}
void File::Close() {
  if (!native->mappings.empty()) throw std::logic_error("Close file after unmapping its views");
  if (native->descriptor >= 0) { ::close(native->descriptor); native->descriptor = -1; }
}
Bool File::IsOpen() { return native->descriptor >= 0; }
Bool File::IsMapped() { return !native->mappings.empty(); }
U32 File::Read(void *dst, U32 count, U32 size) {
  const U32 total = checkedSize(std::uint64_t(count) * size);
  U32 done = 0;
  while (done < total) {
    auto n = ::read(native->descriptor, static_cast<U8 *>(dst) + done, total - done);
    if (n < 0) { if (errno == EINTR) continue; if (!(native->mode & NOERR)) ReadError(total); break; }
    if (!n) break;
    done += static_cast<U32>(n);
  }
  return done; // Original API returns bytes, including a short read at EOF.
}
U32 File::Write(const void *src, U32 count, U32 size) {
  if (IsMapped()) throw std::logic_error("Cannot write through File while mapped");
  const U32 total = checkedSize(std::uint64_t(count) * size);
  U32 done = 0;
  while (done < total) {
    auto n = ::write(native->descriptor, static_cast<const U8 *>(src) + done, total - done);
    if (n < 0 && errno == EINTR) continue;
    if (n <= 0) { if (!(native->mode & NOERR)) WriteError(total); break; }
    done += static_cast<U32>(n);
  }
  if ((native->mode & FLUSH) && !Flush() && !(native->mode & NOERR)) WriteError(total);
  return done;
}
Bool File::WriteU8(U8 v) { return Write(&v, 1) == 1; }
Bool File::WriteU16(U16 v) {
  const U8 b[] = {U8(v), U8(v >> 8)}; return Write(b, sizeof b) == sizeof b;
}
Bool File::WriteU32(U32 v) {
  const U8 b[] = {U8(v), U8(v >> 8), U8(v >> 16), U8(v >> 24)}; return Write(b, sizeof b) == sizeof b;
}
Bool File::WriteString(const char *format, ...) {
  va_list ap, copy; va_start(ap, format); va_copy(copy, ap);
  const int n = std::vsnprintf(nullptr, 0, format, copy); va_end(copy);
  if (n < 0) { va_end(ap); return FALSE; }
  std::vector<char> buf(std::size_t(n) + 1);
  std::vsnprintf(buf.data(), buf.size(), format, ap); va_end(ap);
  return n && Write(buf.data(), static_cast<U32>(n)) == static_cast<U32>(n);
}
Bool File::Seek(U32 origin, S32 offset) {
  if (origin > END) return FALSE;
  const int origins[] = {SEEK_SET, SEEK_CUR, SEEK_END};
  return ::lseek(native->descriptor, offset, origins[origin]) != -1;
}
U32 File::Tell() {
  const auto n = ::lseek(native->descriptor, 0, SEEK_CUR);
  if (n < 0) failure("tell"); return checkedSize(n);
}
Bool File::Flush() { return ::fsync(native->descriptor) == 0; }
U32 File::GetSize() {
  struct stat st{};
  if (::fstat(native->descriptor, &st)) failure("fstat");
  return checkedSize(st.st_size);
}
void *File::MapMemory(const char *, U32 flags, U32 offset, U32 length, void **aligned) {
  if (aligned) *aligned = nullptr;
  const U32 size = GetSize();
  if (offset >= size || (length && length > size - offset))
    throw std::out_of_range("Mapping exceeds file size");
  if (!length) length = size - offset;
  const long page = ::sysconf(_SC_PAGESIZE);
  if (page <= 0) throw std::runtime_error("Cannot determine memory page size");
  const std::size_t baseOffset = offset / page * page;
  const std::size_t delta = offset - baseOffset, mappedLength = std::size_t(length) + delta;
  int prot = PROT_READ | ((flags & MAP_WRITE) ? PROT_WRITE : 0);
  void *base = ::mmap(nullptr, mappedLength, prot, MAP_SHARED, native->descriptor, baseOffset);
  if (base == MAP_FAILED) failure("mmap");
  auto data = static_cast<U8 *>(base) + delta;
  try { native->mappings.push_back({base, data, mappedLength}); }
  catch (...) { ::munmap(base, mappedLength); throw; }
  if (aligned) *aligned = base;
  return data;
}
void File::UnmapMemory(void *pointer) {
  auto &v = native->mappings;
  auto it = std::find_if(v.begin(), v.end(), [pointer](const auto &m) { return m.base == pointer || m.data == pointer; });
  if (it == v.end()) throw std::invalid_argument("Unknown file mapping");
  if (::munmap(it->base, it->length)) failure("munmap");
  v.erase(it);
}
Bool File::Unique(const char *pattern, FileName &out) {
  // Original callers supply an mktemp template ending in six X characters.
  auto s = nativePath(pattern).string();
  if (s.size() >= out.GetSize() || s.size() < 6 || s.substr(s.size()-6) != "XXXXXX") return FALSE;
  std::vector<char> buf(s.begin(), s.end()); buf.push_back(0);
  int fd = ::mkstemp(buf.data()); if (fd < 0) return FALSE;
  ::close(fd); ::unlink(buf.data()); out = buf.data(); return TRUE;
}
Bool File::Exists(const char *d, const char *n) { return Exists(joined(d,n).c_str()); }
Bool File::Exists(const char *p) { try { return fs::exists(nativePath(p)); } catch (...) { return FALSE; } }
Bool File::Unlink(const char *d, const char *n) { return Unlink(joined(d,n).c_str()); }
Bool File::Unlink(const char *p) { try { return ::unlink(nativePath(p).c_str()) == 0; } catch (...) { return FALSE; } }
Bool File::Rename(const char *d, const char *a, const char *b) { return Rename(joined(d,a).c_str(), joined(d,b).c_str()); }
Bool File::Rename(const char *a, const char *b) {
  try { return ::rename(nativePath(a).c_str(), nativePath(b).c_str()) == 0; } catch (...) { return FALSE; }
}
Bool File::Copy(const char *a, const char *b, Bool overwrite, Bool) {
  try { return fs::copy_file(nativePath(a), nativePath(b), overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none); }
  catch (...) { return FALSE; }
}
Bool File::Remove(const char *p) { try { return fs::remove_all(nativePath(p)) != 0; } catch (...) { return FALSE; } }
Bool File::GetAttrib(const char *d, const char *n, U32 &a) { return GetAttrib(joined(d,n).c_str(),a); }
Bool File::GetAttrib(const char *p, U32 &a) { try { a=attributes(nativePath(p)); return TRUE; } catch (...) { return FALSE; } }
Bool File::SetAttrib(const char *d, const char *n, U32 a) { return SetAttrib(joined(d,n).c_str(),a); }
Bool File::SetAttrib(const char *p, U32 a) {
  try {
    auto path = nativePath(p); struct stat st{};
    if (::stat(path.c_str(), &st)) return FALSE;
    // HIDDEN and SYSTEM cannot be toggled as Windows metadata on POSIX.
    if ((a & (HIDDEN | SYSTEM)) != (attributes(path) & (HIDDEN | SYSTEM))) return FALSE;
    mode_t mode = st.st_mode;
    if (a & READONLY) mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH); else mode |= S_IWUSR;
    return ::chmod(path.c_str(), mode) == 0;
  } catch (...) { return FALSE; }
}
U32 File::Crc(const char *d, const char *n, U32 crc) { return Crc(joined(d,n).c_str(),crc); }
U32 File::Crc(const char *p, U32 crc) {
  File f; if (!f.Open(p, READ)) throw std::runtime_error("Cannot open CRC input");
  std::array<U8, 65536> bytes; U32 n;
  while ((n = f.Read(bytes.data(), bytes.size()))) crc = ::Crc::Calc(bytes.data(), n, crc);
  return crc;
}
void File::ReadError(U32) { failure("File read"); }
void File::WriteError(U32) { failure("File write"); }

Bool Dir::PathAbsolute(FilePath &out, const char *p) {
  try { assign(out, fs::absolute(nativePath(p)).lexically_normal()); return TRUE; } catch (...) { return FALSE; }
}
void Dir::PathExpand(const char *p, FileDrive &drive, FileDir &dir, FileName &name, FileExt &ext) {
  auto path = nativePath(p); drive = "";
  auto parent = path.parent_path().string(); if (!parent.empty()) parent += '/';
  dir = parent.c_str(); name = path.stem().c_str(); ext = path.extension().c_str();
}
void Dir::PathMake(FilePath &out, const char *drive, const char *dir, const char *name, const char *ext) {
  if (drive && *drive) throw std::invalid_argument("Native engine has no DOS drive letters");
  std::string filename = name ? name : "";
  if (ext && *ext) { if (*ext != '.') filename += '.'; filename += ext; }
  PathMake(out, dir, filename.c_str());
}
void Dir::PathMake(FilePath &out, const char *dir, const char *name) {
  // Engine path strings are CRC identifiers as well as filesystem paths.
  // Preserve the original backslash spelling; normalize only at OS calls.
  std::string path = dir ? dir : "";
  if (!path.empty() && path.back() != '\\' && path.back() != '/') path += '\\';
  if (name) path += name;
  assign(out, path);
}
Bool Dir::GetCurrent(int drive, FileDir &out) { return drive == 0 ? GetCurrent(out) : FALSE; }
Bool Dir::GetCurrent(FileDir &out) {
  try { auto p = fs::current_path().string(); if (p.size() >= out.GetSize()) return FALSE; out = p.c_str(); return TRUE; }
  catch (...) { return FALSE; }
}
Bool Dir::SetCurrent(const char *p) { try { return ::chdir(nativePath(p).c_str()) == 0; } catch (...) { return FALSE; } }
Dir::Find::~Find() { delete handle; }
Bool Dir::FindFirst(Find &f, const char *d, const char *n, U32 a) { return FindFirst(f, joined(d,n).c_str(), a); }
Bool Dir::FindFirst(Find &f, const char *pattern, U32 a) {
  FindClose(f); f.findattrib = a;
  try {
    std::string s(pattern); std::replace(s.begin(),s.end(),'\\','/');
    fs::path p(s); auto dir = nativePath(p.has_parent_path() ? p.parent_path().c_str() : ".");
    std::string wildcard = fold(p.filename().string());
    if (wildcard == "*.*") wildcard = "*"; // DOS also matches extensionless names.
    auto state = std::make_unique<NativeFindState>();
    for (const auto &entry : fs::directory_iterator(dir)) {
      auto name = entry.path().filename().string();
      bool match = wildcard == "*." ? name.find('.') == std::string::npos :
        ::fnmatch(wildcard.c_str(), fold(name).c_str(), 0) == 0;
      if (match) state->paths.push_back(entry.path());
    }
    std::sort(state->paths.begin(), state->paths.end());
    f.handle = state.release();
    if (FindNext(f)) return TRUE;
    FindClose(f); return FALSE;
  } catch (...) { FindClose(f); return FALSE; }
}
Bool Dir::FindNext(Find &f) {
  if (!f.handle) return FALSE;
  while (f.handle->index < f.handle->paths.size()) {
    const auto &p = f.handle->paths[f.handle->index++];
    try {
      U32 a = attributes(p);
      if ((a & f.findattrib) != f.findattrib) continue; // Original requires all requested bits.
      auto name = p.filename().string();
      if (name.size() >= sizeof f.finddata.name) continue;
      struct stat st{}; if (::stat(p.c_str(), &st)) continue;
      f.finddata = {}; f.finddata.attrib = a;
      f.finddata.size = S_ISDIR(st.st_mode) ? 0 : checkedSize(st.st_size);
      f.finddata.time_create = st.st_ctime; f.finddata.time_access = st.st_atime; f.finddata.time_write = st.st_mtime;
      std::memcpy(f.finddata.name, name.c_str(), name.size() + 1); return TRUE;
    } catch (...) { continue; }
  }
  return FALSE;
}
Bool Dir::FindClose(Find &f) { delete f.handle; f.handle = nullptr; return TRUE; }
Bool Dir::FindPath(char *dst, U32 capacity, const char *input) {
  try {
    auto p = nativePath(input);
    if (fs::exists(p)) { if (p.string().size() >= capacity) return FALSE; Utils::Strmcpy(dst,p.c_str(),capacity); return TRUE; }
    auto root = p.has_parent_path() ? p.parent_path() : fs::path(".");
    // Never follow directory symlinks: avoids recursion loops and leaving the search tree.
    for (const auto &entry : fs::recursive_directory_iterator(root)) {
      if (fold(entry.path().filename().string()) == fold(p.filename().string())) {
        auto s = entry.path().string(); if (s.size() >= capacity) return FALSE;
        Utils::Strmcpy(dst,s.c_str(),capacity); return TRUE;
      }
    }
  } catch (...) {}
  return FALSE;
}
Bool Dir::Make(const char *p) { try { return fs::create_directory(nativePath(p)); } catch (...) { return FALSE; } }
Bool Dir::MakeFull(const char *p) {
  try { auto path=nativePath(p); return fs::create_directories(path) || fs::is_directory(path); } catch (...) { return FALSE; }
}
Bool Dir::Remove(const char *p) { try { return ::rmdir(nativePath(p).c_str()) == 0; } catch (...) { return FALSE; } }
// DOS drive discovery has no native counterpart. Callers must use the imported
// data root, not a pretend CD drive. Failure is explicit.
Bool Drive::FindDriveByVolume(Type, const char *, int &) { return FALSE; }
Drive::Type Drive::GetType(int) { return NONEXIST; }
int Drive::GetCurrent() { return -1; }
Bool Drive::SetCurrent(int) { return FALSE; }

U32 Dir::Crc(const char *input) {
  // Preserve original breadth-first traversal with per-directory CRC-key order.
  std::vector<fs::path> queue{nativePath(input)};
  U32 crc = 0;
  for (std::size_t i = 0; i < queue.size(); ++i) {
    auto p = queue[i];
    if (fs::is_symlink(p)) throw std::runtime_error("Directory CRC does not follow symlinks");
    if (!fs::is_directory(p)) { crc = File::Crc(p.c_str(), crc); continue; }
    std::vector<fs::path> children;
    for (const auto &entry : fs::directory_iterator(p)) {
      auto name = entry.path().filename().string();
      if (!name.empty() && name.back() != '.') children.push_back(entry.path());
    }
    std::sort(children.begin(), children.end(), [](const auto &a, const auto &b) {
      auto ac = ::Crc::CalcStr(a.filename().c_str()), bc = ::Crc::CalcStr(b.filename().c_str());
      return ac == bc ? a < b : ac < bc;
    });
    queue.insert(queue.end(), children.begin(), children.end());
  }
  return crc;
}
