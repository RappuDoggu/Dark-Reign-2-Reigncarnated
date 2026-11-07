#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include "reign/platform.hpp"
int engine_checks();
namespace {
struct Shell {
 reign::Lifecycle lifecycle;
 EGLDisplay display = EGL_NO_DISPLAY;
 EGLSurface surface = EGL_NO_SURFACE;
 EGLContext context = EGL_NO_CONTEXT;
 bool passed = false;
 void release() {
  if (display != EGL_NO_DISPLAY) {
   eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
   if (context != EGL_NO_CONTEXT) eglDestroyContext(display, context);
   if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
   eglTerminate(display);
  }
  display = EGL_NO_DISPLAY; surface = EGL_NO_SURFACE; context = EGL_NO_CONTEXT;
  lifecycle.surface(false);
 }
 bool create(ANativeWindow *window) {
  release();
  display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (display == EGL_NO_DISPLAY || !eglInitialize(display, nullptr, nullptr)) return false;
  if (!eglBindAPI(EGL_OPENGL_ES_API)) return false;
  const EGLint attrs[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE,
   EGL_OPENGL_ES3_BIT, EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_NONE};
  EGLConfig config; EGLint count = 0, format = 0;
  if (!eglChooseConfig(display, attrs, &config, 1, &count) || count != 1) return false;
  if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format)) return false;
  if (ANativeWindow_setBuffersGeometry(window, 0, 0, format) != 0) return false;
  const EGLint ctx[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
  context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctx);
  surface = eglCreateWindowSurface(display, config, window, nullptr);
  if (context == EGL_NO_CONTEXT || surface == EGL_NO_SURFACE ||
      !eglMakeCurrent(display, surface, surface, context)) return false;
  lifecycle.surface(true);
  __android_log_print(ANDROID_LOG_INFO, "Reigncarnated", "GLES initialized: %s", glGetString(GL_VERSION));
  return true;
 }
 void draw() {
  if (lifecycle.state() != reign::State::running) return;
  EGLint w = 0, h = 0;
  if (!eglQuerySurface(display, surface, EGL_WIDTH, &w) ||
      !eglQuerySurface(display, surface, EGL_HEIGHT, &h)) return;
  glViewport(0, 0, w, h);
  // Green: original primitive self-tests passed. Red: failed. This is NOT a game scene.
  glClearColor(passed ? 0.03f : 0.5f, passed ? 0.25f : 0.01f, 0.07f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  if (!eglSwapBuffers(display, surface))
   __android_log_print(ANDROID_LOG_ERROR, "Reigncarnated", "eglSwapBuffers failed: %x", eglGetError());
 }
};
Shell &get(ANativeActivity *a) { return *static_cast<Shell *>(a->instance); }
void resume(ANativeActivity *a) { get(a).lifecycle.resume(); get(a).draw(); }
void pause(ANativeActivity *a) { get(a).lifecycle.pause(); }
void created(ANativeActivity *a, ANativeWindow *window) {
 auto &s = get(a);
 if (!s.create(window)) {
  __android_log_print(ANDROID_LOG_ERROR, "Reigncarnated", "EGL initialization failed: %x", eglGetError());
  s.release(); return;
 }
 s.draw();
}
void redraw(ANativeActivity *a, ANativeWindow *) { get(a).draw(); }
void removed(ANativeActivity *a, ANativeWindow *) { get(a).release(); }
void destroy(ANativeActivity *a) {
 auto *s = static_cast<Shell *>(a->instance);
 s->lifecycle.destroy(); s->release(); delete s; a->instance = nullptr;
}
}
extern "C" JNIEXPORT void ANativeActivity_onCreate(ANativeActivity *a, void *, size_t) {
 auto *s = new Shell;
 a->instance = s;
 s->passed = engine_checks() == 0;
 __android_log_print(ANDROID_LOG_INFO, "Reigncarnated", "Original CRC/RNG/PACK tests: %s; full engine not initialized", s->passed ? "PASS" : "FAIL");
 a->callbacks->onResume = resume;
 a->callbacks->onPause = pause;
 a->callbacks->onNativeWindowCreated = created;
 a->callbacks->onNativeWindowResized = redraw;
 a->callbacks->onNativeWindowRedrawNeeded = redraw;
 a->callbacks->onNativeWindowDestroyed = removed;
 a->callbacks->onDestroy = destroy;
}
