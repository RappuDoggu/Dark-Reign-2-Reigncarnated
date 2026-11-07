#include "reign/config.hpp"
#include "reign/archive.hpp"
#include "reign/engine_file.hpp"
#include "reign/shell_camera.hpp"
#include <cmath>
#include <iostream>
int main(int argc,char **argv){try{
 if(argc!=3)throw std::runtime_error("Usage: shell_camera_probe base.zwp shell.zwp");
 reign::Archive base(argv[1]),shell(argv[2]);auto loader=[&](const std::string &name){auto at=name.find_last_of("/\\");auto key=at==std::string::npos?name:name.substr(at+1);if(shell.find(key))return shell.read(key);if(base.find(key))return base.read(key);throw FileSys::FileNotFound(key);};
 auto objects=reign::parse_config(loader,"objects.cfg");unsigned bookmarks=0,samples=0;
 for(auto &object:objects)for(auto &scope:object.children)if(scope.name=="BookmarkObj"){
  reign::ShellCamera camera(scope);++bookmarks;
  std::cout<<camera.name()<<" duration="<<camera.duration()<<" loop="<<camera.looping()<<" focus="<<camera.has_focus()<<'\n';
  for(unsigned i=0;i<120;++i){auto s=camera.sample(camera.duration()*i/120);if(!s.active)throw std::runtime_error("Premature camera completion: "+camera.name());++samples;}
  auto end=camera.sample(camera.duration());if(end.active!=camera.looping())throw std::runtime_error("Camera completion mismatch");
  if(camera.name()=="Main::Enter")for(float t:{0.f,.5f,1.5f,3.f,4.f,5.5f}){auto s=camera.sample(t);std::cout<<"t="<<t<<" camera="<<s.position[0]<<','<<s.position[1]<<','<<s.position[2]<<" direction="<<s.direction[0]<<','<<s.direction[1]<<','<<s.direction[2]<<'\n';}
 }
 if(bookmarks==0)throw std::runtime_error("No shell bookmarks");
 std::cout<<"Original BookmarkObj Curve runtime: "<<bookmarks<<" bookmarks, "<<samples<<" active samples. World rendering and cinematic scheduler are not yet connected.\n";
}catch(const std::exception &e){std::cerr<<"SHELL_CAMERA_FAIL "<<e.what()<<'\n';return 1;}}
