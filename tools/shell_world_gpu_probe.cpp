#include "reign/shell_camera.hpp"
#include "reign/god_pose.hpp"
#include <map>
#include <algorithm>
#include "reign/shell_terrain.hpp"
#include "reign/world_projection.hpp"
#include "reign/legacy_gles.hpp"
#include "reign/resources.hpp"
#include "reign/bitmap_manager.hpp"
#include "reign/engine_file.hpp"
#include "terrain_probe_tables.hpp"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <fstream>
#include <iostream>
#include <cmath>
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
// Read-only developer scene integration. Original PTree resolves includes;
// this is not GameObjCtrl, MapObj simulation, or the cinematic scheduler.
namespace {
const reign::ConfigNode *child(const reign::ConfigNode &n,const std::string &name){for(auto &c:n.children)if(c.name==name)return &c;return nullptr;}
std::string folded(std::string s){for(char &c:s)if(c>='A'&&c<='Z')c+=32;return s;}
float number(const reign::ConfigValue &v){if(auto f=std::get_if<float>(&v))return *f;return float(std::get<std::int32_t>(v));}
struct Model {
 reign::GodMesh mesh;reign::GodPoseEvaluator pose;const reign::GodCycle *cycle=nullptr;
 Model(reign::ConfigLoader loader,const std::string &name):mesh(reign::read_god_mesh(loader,name)),pose(mesh){for(auto &c:mesh.cycles)if(folded(c.name)=="default")cycle=&c;}
};
struct Placement{Model *model;reign::PoseMatrix world;int id,parent=-1;std::string attach;};
}
int main(int argc,char **argv){try{
 if(argc!=3)throw std::invalid_argument("Usage: shell_world_gpu_probe installation-root output-prefix");
 reign::ResourceStore sources(argv[1]);for(auto *s:{"mission","world","side"})sources.add_stream("gamedata",s);
 if(!sources.add_archive("gamedata","packs/base.zwp")&&!sources.add_archive("gamedata","base.zwp"))throw std::runtime_error("Missing base archive");
 if(!sources.add_archive("mission","missions/system/shell.zwp")&&!sources.add_archive("mission","shell.zwp"))throw std::runtime_error("Missing shell archive");
 if(!sources.add_archive("world","worlds/sprawl.zwp"))throw std::runtime_error("Missing Sprawl world archive");
 auto loader=[&](const std::string &name){return sources.load("gamedata",name);};
 unsigned width=0,height=0;for(auto &scope:reign::parse_config(loader,"game.cfg"))if(scope.name=="WorldInfo")for(auto &c:scope.children){if(c.name=="CellMapX")width=std::get<std::int32_t>(c.arguments.at(0));if(c.name=="CellMapZ")height=std::get<std::int32_t>(c.arguments.at(0));}

 std::map<std::string,std::string> definitions;
 for(const char *file:{"types_objects.cfg","types_world.cfg"})for(auto &type:reign::parse_config(loader,file))if(type.name=="CreateObjectType")if(auto map=child(type,"MapObj"))if(auto god=child(*map,"GodFile")){
  auto id=folded(std::get<std::string>(type.arguments.at(0)));if(!definitions.emplace(id,std::get<std::string>(god->arguments.at(0))).second)throw std::runtime_error("Duplicate model type: "+id);
 }
 std::map<std::string,std::unique_ptr<Model>> models;std::vector<Placement> placements;
 for(auto &object:reign::parse_config(loader,"objects.cfg"))if(object.name=="CreateObject")if(auto map=child(object,"MapObj")){
  auto type=folded(std::get<std::string>(object.arguments.at(0)));auto found=definitions.find(type);if(found==definitions.end())throw std::runtime_error("No direct GodFile for shell type: "+type);
  auto name=folded(found->second);auto &model=models[name];if(!model)model=std::make_unique<Model>(loader,name);
  auto parent=child(*map,"Parent");auto position=child(*map,parent?"Orientation":"Position");if(!position)throw std::runtime_error("Shell object has no position/orientation");auto world=reign::identity_pose_matrix;
  for(auto field:std::initializer_list<std::pair<const char*,unsigned>>{{"Right",0},{"Up",4},{"Front",8},{"Pos",12}}){auto vector=child(*position,field.first);if(!vector||vector->arguments.size()!=3)throw std::runtime_error("Invalid shell placement vector");for(unsigned i=0;i<3;++i)world[field.second+i]=number(vector->arguments[i]);}
  Placement placed{model.get(),world,std::get<std::int32_t>(object.arguments.at(1))};if(parent){placed.parent=std::get<std::int32_t>(parent->arguments.at(0));auto point=child(*map,"AttachPoint");if(!point)throw std::runtime_error("Missing attachment point");placed.attach=folded(std::get<std::string>(point->arguments.at(0)));}if(std::any_of(placements.begin(),placements.end(),[&](auto &p){return p.id==placed.id;}))throw std::runtime_error("Duplicate shell object ID");placements.push_back(std::move(placed));
 }
 if(placements.empty())throw std::runtime_error("No shell model placements");
 std::cout<<"Original shell placements="<<placements.size()<<" unique-models="<<models.size()<<'\n';
 auto terrain=reign::load_shell_terrain(loader,width,height);std::unique_ptr<reign::ShellCamera> camera;
 for(auto &object:reign::parse_config(loader,"objects.cfg"))for(auto &b:object.children)if(b.name=="BookmarkObj"&&std::get<std::string>(b.arguments.at(0))=="Main::Enter")camera=std::make_unique<reign::ShellCamera>(b);
 if(!camera)throw std::runtime_error("Missing Main::Enter camera");
 // Match the original setup's extra bottom row; recalculated normals/lighting are
 // outside this diagnostic, which displays unlit terrain and posed models.
 terrain.cells.resize((height+1)*terrain.pitch);for(unsigned x=0;x<terrain.pitch;++x){auto &c=terrain.cells[height*terrain.pitch+x];c.height=10;c.color=0xffffffff;}
 std::vector<std::vector<std::uint8_t>> reference;
 for(unsigned generation=0;generation<2;++generation){
  Context context;reign::LegacyGles renderer(resolve);
  reign::BitmapManager bitmaps(loader,[&](const std::string &name){try{loader(name);return true;}catch(const FileSys::FileNotFound&){return false;}},
   {[&](const reign::RgbaImage &image){return renderer.texture_rgba(image.width,image.height,image.pixels.data());},[&](unsigned texture){renderer.delete_texture(texture);},[]{}});
  std::vector<unsigned> textures;for(auto &name:terrain.textures)textures.push_back(bitmaps.load(name)->texture);
  unsigned frame=0;
  for(float seconds:{0.f,3.f,5.5f}){
   auto sample=camera->sample(seconds);if(!sample.active)throw std::runtime_error("Camera sample inactive");
   // Diagnostic frustum: original projection implementation, explicit 60deg/1/1000
   // values. Final viewer/settings/performance-frustum policy remains unbound.
   reign::WorldProjection projection(sample.position,sample.direction,1.0471975512f,float(H)/W,1,1000);
   struct Batch{std::vector<reign::LegacyClip> vertices;std::vector<std::uint16_t> indices;};std::vector<Batch> batches(textures.size());
   for(unsigned cz=0;cz<height;cz+=4)for(unsigned cx=0;cx<width;cx+=4){
    std::array<std::array<float,4>,25> clip{};
    for(unsigned z=0;z<5;++z)for(unsigned x=0;x<5;++x){auto &cell=terrain.cells[(cz+z)*terrain.pitch+cx+x];clip[z*5+x]=projection.transform({float((cx+x)*8)-float(int(terrain.offsetX)),cell.height,float((cz+z)*8)-float(int(terrain.offsetZ))});}
    for(unsigned z=0;z<4;++z)for(unsigned x=0;x<4;++x){auto &cell=terrain.cells[(cz+z)*terrain.pitch+cx+x];if(cell.texture>=batches.size()||cell.uv>44)throw std::runtime_error("Invalid terrain material or UV table offset");auto &batch=batches[cell.texture];
     const unsigned corners[]={z*5+x,(z+1)*5+x,(z+1)*5+x+1,z*5+x+1};
     for(unsigned triangle=0;triangle<6;++triangle){unsigned source=reign::world_detail::clusterI[(z*4+x)*6+triangle],corner=0;while(corner<4&&corners[corner]!=source)++corner;if(corner==4)throw std::runtime_error("Original cluster topology mismatch");auto &uv=reign::world_detail::cellUVList[cell.uv+corner];auto &p=clip[source];
      if(batch.vertices.size()>=65536)throw std::runtime_error("Probe batch exceeds 16-bit indices");batch.indices.push_back(std::uint16_t(batch.vertices.size()));batch.vertices.push_back({p[0],p[1],p[2],p[3],0xffffffff,0xff000000,uv.u,uv.v});
     }
    }
   }
   renderer.clear(W,H,0,0,0);reign::DrawState state;state.depth_test=true;state.depth_write=true;state.filtered=true;state.texture_op=reign::TextureOp::texture;
   unsigned triangles=0;for(unsigned i=0;i<batches.size();++i){auto &b=batches[i];if(b.vertices.empty())continue;state.texture=textures[i];renderer.draw_clip(W,H,b.vertices.data(),b.vertices.size(),b.indices.data(),b.indices.size(),state);triangles+=b.indices.size()/3;}

   std::vector<reign::GodPoseFrame> poses(placements.size());std::vector<unsigned> poseStatus(placements.size());
   std::function<void(std::size_t)> evaluate=[&](std::size_t i){if(poseStatus[i]==2)return;if(poseStatus[i]==1)throw std::runtime_error("Circular shell attachment");poseStatus[i]=1;auto &placed=placements[i];auto &model=*placed.model;auto world=placed.world;
    if(placed.parent>=0){auto parent=std::find_if(placements.begin(),placements.end(),[&](auto &p){return p.id==placed.parent;});if(parent==placements.end())throw std::runtime_error("Missing shell attachment parent");auto pi=std::size_t(parent-placements.begin());evaluate(pi);auto &nodes=parent->model->mesh.nodes;auto node=std::find_if(nodes.begin(),nodes.end(),[&](auto &n){return folded(n.name)==placed.attach;});if(node==nodes.end())throw std::runtime_error("Missing shell attachment node");world=reign::compose_pose_matrices(world,poses[pi].worldMatrices.at(node->state));}
    float frame=0;if(model.cycle&&model.cycle->maxFrame>0)frame=std::fmod(seconds*model.cycle->speed,model.cycle->maxFrame);
    // Explicit diagnostic frame mapping, not MeshEnt clock/cinematic scheduling.
    poses[i]=model.pose.sample(model.cycle?model.cycle->name:"",frame,world);poseStatus[i]=2;
   };for(std::size_t i=0;i<placements.size();++i)evaluate(i);
   unsigned modelTriangles=0;
   for(std::size_t object=0;object<placements.size();++object){auto &model=*placements[object].model;auto &pose=poses[object];auto &mesh=model.mesh.geometry;
    std::vector<Batch> modelBatches(mesh.buckets.size());
    for(auto &face:mesh.faces){auto &batch=modelBatches.at(face.bucket);for(unsigned i=0;i<3;++i){auto p=projection.transform(pose.vertices.at(face.vertices[i]));auto uv=mesh.uvs.at(face.uvs[i]);if(batch.vertices.size()>=65536)throw std::runtime_error("Model batch too large");batch.indices.push_back(std::uint16_t(batch.vertices.size()));batch.vertices.push_back({p[0],p[1],p[2],p[3],0xffffffff,0xff000000,uv[0],uv[1]});}}
    for(unsigned i=0;i<modelBatches.size();++i){auto &batch=modelBatches[i];if(batch.indices.empty())continue;auto &texture=mesh.buckets[i].texture0;state.texture=texture&&!texture->name.empty()?bitmaps.load(texture->name)->texture:0;state.texture_op=state.texture?reign::TextureOp::texture:reign::TextureOp::diffuse;renderer.draw_clip(W,H,batch.vertices.data(),batch.vertices.size(),batch.indices.data(),batch.indices.size(),state);modelTriangles+=batch.indices.size()/3;}
   }
   renderer.present();std::vector<std::uint8_t> pixels(W*H*4);renderer.read_presented_rgba(W,H,pixels.data());unsigned nonblack=0;for(unsigned i=0;i<W*H;++i)if(pixels[i*4]||pixels[i*4+1]||pixels[i*4+2])++nonblack;if(nonblack<1000)throw std::runtime_error("World diagnostic unexpectedly empty");
   if(generation==0){if(!reference.empty()&&pixels==reference.back())throw std::runtime_error("Camera advance did not change world frame");reference.push_back(pixels);std::ofstream out(std::string(argv[2])+"-"+std::to_string(frame)+".ppm",std::ios::binary);out<<"P6\n"<<W<<' '<<H<<"\n255\n";for(unsigned row=0;row<H;++row)for(unsigned x=0;x<W;++x)out.write(reinterpret_cast<const char*>(&pixels[((H-1-row)*W+x)*4]),3);if(!out)throw std::runtime_error("Snapshot write failed");}
   else if(pixels!=reference.at(frame))throw std::runtime_error("World frame changed across context recreation");
   std::cout<<"Context "<<generation+1<<" t="<<seconds<<" triangles="<<triangles<<" model-triangles="<<modelTriangles<<" pixels="<<nonblack<<'\n';++frame;
  }
 }
 std::cout<<"Original camera/terrain/model-pose GLES integration: PASS. UNLIT DIAGNOSTIC ONLY; overlays, water, lighting, material policy and cinematic runtime absent.\n";
}catch(const std::exception &e){std::cerr<<"SHELL_WORLD_GPU_FAIL "<<e.what()<<'\n';return 1;}}
