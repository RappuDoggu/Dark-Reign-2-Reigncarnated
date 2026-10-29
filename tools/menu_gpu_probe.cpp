#include "reign/menu_frontend.hpp"
#include "reign/menu_audio.hpp"
#include "reign/shell_scene.hpp"
#include "reign/frontend_presenter.hpp"
#include "reign/legacy_gles.hpp"
#include "reign/archive.hpp"
#include "reign/engine_file.hpp"
#include "reign/platform.hpp"
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
 if(argc!=3&&argc!=4&&argc!=5)throw std::invalid_argument("Usage: menu_gpu_probe base.zwp output.ppm [shell.zwp]");
 reign::Archive archive(argv[1]);std::unique_ptr<reign::Archive> shell;if(argc>=4)shell=std::make_unique<reign::Archive>(argv[3]);auto loader=[&](const std::string &name){auto pos=name.find_last_of("/\\");auto key=pos==std::string::npos?name:name.substr(pos+1);if(shell&&shell->find(key))return shell->read(key);if(!archive.find(key))throw FileSys::FileNotFound(key);return archive.read(key);};
 auto root=std::filesystem::temp_directory_path()/("reign-menu-gpu-"+std::to_string(reign::monotonic_ns()));struct Cleanup{std::filesystem::path p;~Cleanup(){std::filesystem::remove_all(p);}} cleanup{root};
 reign::MenuAudio audio(loader,root/"audio");reign::MenuFrontend menu(loader,root,W,H,[&](const std::string &name){return audio.load(name);});if(argc==5){menu.open_shell_preview();}else if(shell){menu.create_profile("GPU Pilot");menu.login_selected_profile();}std::vector<std::uint8_t> previous,pixels(W*H*4);std::vector<std::vector<std::uint8_t>> sceneFrames;
 for(unsigned generation=0;generation<2;++generation){Context context;reign::LegacyGles renderer(resolve);reign::FrontendPresenter presenter(renderer);std::unique_ptr<reign::ShellScene> scene;if(argc==5){scene=std::make_unique<reign::ShellScene>(argv[4],renderer);if(!scene->ambient_sound().empty())std::cout<<"Weather WAVs validated: "<<audio.cached_path("engine_thunder.wav")<<" "<<audio.cached_path(scene->ambient_sound())<<'\n';}
  for(unsigned frame=0;frame<(scene?7u:2u);++frame){presenter.begin(W,H);if(scene)scene->draw(std::array<float,7>{0.f,3.f,5.5f,6.5f,9.5f,13.6f,14.5f}[frame]);reign::MenuFrameReport report;if(!scene||frame>=2)report=menu.draw([&](const auto &batch){presenter.submit(batch);},[&](const auto &font,const auto &batch){presenter.submit(font,batch);},0,0);if(scene)scene->finish_frame(std::array<float,7>{0.f,3.f,5.5f,6.5f,9.5f,13.6f,14.5f}[frame]);presenter.present();renderer.read_presented_rgba(W,H,pixels.data());
   if((shell?!menu.control_active("Shell::Main"):!report.login_active)||(scene?report.backdrop_active:!report.backdrop_active)||((!scene||frame>=2)&&!report.font_batches))throw std::runtime_error("Original Login not active/drawn");
   if(!scene&&!previous.empty()&&pixels!=previous)throw std::runtime_error("Static original menu frame changed across frame/context recreation");previous=pixels;
   if(scene){if(!generation){if(frame>0&&frame<3&&pixels==sceneFrames.back())throw std::runtime_error("Scene did not move");sceneFrames.push_back(pixels);}else if(pixels!=sceneFrames.at(frame))throw std::runtime_error("Scene/menu context recovery mismatch");}
   if(scene){auto sounds=scene->take_sounds();if(frame==5&&sounds.size()!=1)throw std::runtime_error("Lightning did not queue thunder");for(auto &sound:sounds){audio.load(sound)(1.f);std::cout<<"Weather sound: "<<sound<<'\n';}auto s=scene->effect_stats();std::cout<<"Effects: "<<s.emitters<<" emitters, "<<s.particles<<" particles, "<<s.waterClusters<<" water clusters, "<<s.groundSplats<<" ground splats, "<<s.waterSplats<<" water splats\n";if(!s.emitters)throw std::runtime_error("Shell emitter definitions not connected");}
   unsigned nonblack=0;for(unsigned i=0;i<W*H;++i)if(pixels[i*4]||pixels[i*4+1]||pixels[i*4+2])++nonblack;
   if((!scene||frame>0)&&nonblack<W*H/10)throw std::runtime_error("Menu frame unexpectedly empty");
   std::cout<<"Context "<<generation+1<<" frame "<<frame+1<<": "<<report.geometry_batches<<" geometry, "<<report.font_batches<<" fonts, "<<nonblack<<" nonblack pixels\n";
  }
 }
 if(argc==5){auto point=menu.control_center("Shell::Main.InstantAction");menu.take_action();menu.pointer_move(point[0],point[1]);menu.pointer_button(true,point[0],point[1]);menu.pointer_button(false,point[0],point[1]);if(menu.take_action()!="@instant")throw std::runtime_error("Original Instant Action button did not request the native map chooser");std::cout<<"Original Instant Action button -> native chooser: PASS\n";}
 std::ofstream out(argv[2],std::ios::binary);out<<"P6\n"<<W<<' '<<H<<"\n255\n";for(unsigned row=0;row<H;++row)for(unsigned x=0;x<W;++x)out.write(reinterpret_cast<const char *>(&pixels[((H-1-row)*W+x)*4]),3);if(!out)throw std::runtime_error("Snapshot write failed");menu.close();
 std::cout<<"Original menu GPU rendering and context recreation: PASS\n";
}catch(const std::exception &e){std::cerr<<"MENU_GPU_FAIL "<<e.what()<<'\n';return 1;}}

