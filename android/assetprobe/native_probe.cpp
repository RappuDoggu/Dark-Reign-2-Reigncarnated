#include "reign/archive.hpp"
#include "reign/platform.hpp"
#include <jni.h>
#include <sstream>
#include <exception>
extern "C" JNIEXPORT jstring JNICALL
Java_org_reigncarnated_assetprobe_ProbeActivity_inspectNative(JNIEnv *env, jobject, jstring path) {
 if (!path) return env->NewStringUTF("TEST FAILED: no path");
 const char *raw = env->GetStringUTFChars(path, nullptr);
 if (!raw) return nullptr;
 std::string filename(raw);
 env->ReleaseStringUTFChars(path, raw);
 std::string report;
 try {
  const auto started = reign::monotonic_ns();
  reign::Archive archive(filename);
  std::uint64_t bytes=0;
  std::size_t configs=0,meshes=0,textures=0;
  for (const auto &entry : archive.entries()) {
   const auto payload = archive.read(entry.name);
   bytes += payload.size();
   const auto ext = std::filesystem::path(entry.name).extension().string();
   configs += ext == ".cfg"; meshes += ext == ".god";
   textures += ext == ".pic" || ext == ".tga" || ext == ".bmp";
  }
  std::ostringstream out;
  out << "PASS — native ARM64 archive access\n"
      << "Archive entries: " << archive.entries().size() << "\n"
      << "Payload bytes read: " << bytes << "\n"
      << "Configuration files: " << configs << "\n"
      << "GOD meshes: " << meshes << "\nTextures: " << textures << "\n"
      << "Elapsed milliseconds: " << (reign::monotonic_ns()-started)/1000000 << "\n"
      << "Engine startup/gameplay: not tested\n"
      << "This APK contains original porting tools only; no game code or assets.";
  report = out.str();
 } catch (const std::exception &e) { report = std::string("TEST FAILED\n") + e.what(); }
 return env->NewStringUTF(report.c_str());
}
