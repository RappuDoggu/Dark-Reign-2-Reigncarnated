#include "reign/menu_frontend.hpp"
#include "reign/menu_audio.hpp"
#include "reign/shell_scene.hpp"
#include "reign/frontend_presenter.hpp"
#include "reign/legacy_gles.hpp"
#include "reign/archive.hpp"
#include "reign/engine_file.hpp"
#include "reign/platform.hpp"
#include "reign/shell_menu_navigation.hpp"
#include "reign/mission_menu_catalog.hpp"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <fstream>
#include <iostream>
namespace {
constexpr unsigned W=640,H=480;
void *resolve(const char *name){return reinterpret_cast<void *>(eglGetProcAddress(name));}
struct Context {
 EGLDisplay display=EGL_NO_DISPLAY;EGLSurface surface=EGL_NO_SURFACE;EGLContext context=EGL_NO_CONTEXT;
 Context(){auto platform=reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));if(!platform)throw std::runtime_error("No surfaceless EGL entry point");display=platform(EGL_PLATFORM_SURFACELESS_MESA,nullptr,nullptr);
 if(display==EGL_NO_DISPLAY||!eglInitialize(display,nullptr,nullptr)||!eglBindAPI(EGL_OPENGL_ES_API))throw std::runtime_error("EGL initialization failed");
 EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT_KHR,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,16,EGL_NONE};EGLConfig config;EGLint count=0;
 if(!eglChooseConfig(display,attrs,&config,1,&count)||count!=1)throw std::runtime_error("No GLES3 configuration");
 EGLint size[]={EGL_WIDTH,W,EGL_HEIGHT,H,EGL_NONE};surface=eglCreatePbufferSurface(display,config,size);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};context=eglCreateContext(display,config,EGL_NO_CONTEXT,ca);
 if(surface==EGL_NO_SURFACE||context==EGL_NO_CONTEXT||!eglMakeCurrent(display,surface,surface,context))throw std::runtime_error("GLES3 context creation failed");}
 ~Context(){eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);if(context!=EGL_NO_CONTEXT)eglDestroyContext(display,context);if(surface!=EGL_NO_SURFACE)eglDestroySurface(display,surface);if(display!=EGL_NO_DISPLAY)eglTerminate(display);}
};
}
int main(int argc,char **argv){try{
 if(argc!=3)throw std::invalid_argument("Usage: instant_setup_gpu_probe installation output.ppm");
 const std::filesystem::path data=argv[1];reign::Archive base((data/"packs/base.zwp").string()),shell((data/"missions/system/shell.zwp").string());
 auto loader=[&](const std::string &name){auto pos=name.find_last_of("/\\");auto key=pos==std::string::npos?name:name.substr(pos+1);if(shell.find(key))return shell.read(key);if(!base.find(key))throw FileSys::FileNotFound(key);return base.read(key);};
 auto root=std::filesystem::temp_directory_path()/("reign-instant-gpu-"+std::to_string(reign::monotonic_ns()));struct Cleanup{std::filesystem::path p;~Cleanup(){std::filesystem::remove_all(p);}} cleanup{root};
 // The reduced reference archive has compressed PCM unsupported by MenuAudio.
 // This probe validates pixels/control routing only; production audio is unchanged.
 std::cout<<"Visual-only probe: no audio decoding or playback validation\n";
 reign::MenuFrontend menu(loader,root,W,H,[](const std::string &){return [](float){};});menu.use_original_instant_setup([&](const reign::MenuMission &map){return reign::menu_mission_loader(data,map);});menu.open_shell_preview();
 auto maps=reign::menu_missions(data,false);if(maps.empty())throw std::runtime_error("Reference installation has no Instant Action maps");
 auto objectives=reign::parse_config(loader,"objectives_cineractives.cfg");std::vector<std::uint8_t> previous,pixels(W*H*4);
 for(unsigned generation=0;generation<2;++generation){
  Context context;reign::LegacyGles renderer(resolve);reign::FrontendPresenter presenter(renderer);reign::ShellScene scene(data.string(),renderer);reign::ShellMenuNavigation nav(objectives);
  menu.show_main();menu.pointer_move(-100,-100);menu.prepare_instant_setup(maps,{"","Easy","Hard"},0,1,1,123);
  auto path=nav.begin_instant(7,true);scene.select_camera(path,7);menu.hide_shell_controls();
  for(float now:{7.f,8.5f,9.f,10.5f}){
   presenter.begin(W,H);scene.draw(now);if(nav.advance(now)==reign::ShellMenuNavigation::Event::ShowInstant)menu.show_instant_setup();
   reign::MenuFrameReport report;if(nav.page()==reign::ShellMenuNavigation::Page::Instant)report=menu.draw([&](const auto &b){presenter.submit(b);},[&](const auto &font,const auto &b){presenter.submit(font,b);},unsigned(now*1000),16);
   scene.finish_frame(now);presenter.present();renderer.read_presented_rgba(W,H,pixels.data());
   if(now>=9&&(!report.font_batches||!menu.control_active("GameSetup::Instant")||menu.control_active("Shell::Main")))throw std::runtime_error("Instant setup was not drawn/activated");
   if(now==9&&scene.camera_finished(now))throw std::runtime_error("Setup should activate while the city camera is still moving");
   std::cout<<"Context "<<generation+1<<" t="<<now<<" camera="<<scene.camera_name(now)<<" fonts="<<report.font_batches<<" geometry="<<report.geometry_batches<<'\n';
  }
  if(generation&&pixels!=previous)throw std::runtime_error("Instant setup differs after GPU context recreation");previous=pixels;
  auto click=[&](const std::string &name){auto p=menu.control_center(name);menu.pointer_move(p[0],p[1]);menu.pointer_button(true,p[0],p[1]);menu.pointer_button(false,p[0],p[1]);};
  click("GameSetup::Instant.GameConfigMenu.Map");if(!menu.control_active("GameConfigHostOptions"))throw std::runtime_error("Original Map button did not open map chooser");
  click("GameConfigHostOptions.Done");if(menu.control_active("GameConfigHostOptions"))throw std::runtime_error("Map chooser did not close");
  click("GameSetup::Instant.Launch");if(menu.take_action()!="@instant-launch"||menu.instant_selection().opponents!=1||menu.instant_selection().seed!=123)throw std::runtime_error("Native setup launch did not preserve selected options");
  click("GameSetup::Instant.Back");if(menu.take_action()!="@instant-back")throw std::runtime_error("Native setup Back action missing");
  scene.select_camera(nav.return_main(11),11);menu.hide_shell_controls();if(nav.advance(13)!=reign::ShellMenuNavigation::Event::ShowMain)throw std::runtime_error("Original return event missing");menu.show_main();if(!menu.control_active("Shell::Main")||menu.control_active("GameSetup::Instant"))throw std::runtime_error("Main menu not restored");
 }
 std::ofstream out(argv[2],std::ios::binary);out<<"P6\n"<<W<<' '<<H<<"\n255\n";for(unsigned row=0;row<H;++row)for(unsigned x=0;x<W;++x)out.write(reinterpret_cast<const char *>(&pixels[((H-1-row)*W+x)*4]),3);if(!out)throw std::runtime_error("Cannot write snapshot");menu.close();
 std::cout<<"Native Instant Action city, visible original-engine controls, launch/Back and GPU recreation PASS\n";
}catch(const std::exception &e){std::cerr<<"INSTANT_GPU_FAIL "<<e.what()<<'\n';return 1;}}
