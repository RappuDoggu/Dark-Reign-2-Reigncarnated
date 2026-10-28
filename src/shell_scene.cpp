#include "reign/shell_scene.hpp"
#include "reign/shell_camera.hpp"
#include "reign/shell_lighting.hpp"
#include "reign/god_pose.hpp"
#include "reign/shell_terrain.hpp"
#include "reign/world_projection.hpp"
#include "reign/legacy_gles.hpp"
#include "reign/resources.hpp"
#include "reign/bitmap_manager.hpp"
#include "reign/engine_file.hpp"
#include "terrain_probe_tables.hpp"
#ifdef REIGN_BINK
#include "reign/bink_movie.hpp"
#endif
#include <map>
#include <algorithm>
#include <cmath>
namespace {
constexpr unsigned W=640,H=480;
const reign::ConfigNode *child(const reign::ConfigNode &n,const std::string &name){for(auto &c:n.children)if(c.name==name)return &c;return nullptr;}
std::string folded(std::string s){for(char &c:s)if(c>='A'&&c<='Z')c+=32;return s;}
float number(const reign::ConfigValue &v){if(auto f=std::get_if<float>(&v))return *f;return float(std::get<std::int32_t>(v));}
struct Model {
 reign::GodMesh mesh;reign::GodPoseEvaluator pose;const reign::GodCycle *cycle=nullptr;
 Model(reign::ConfigLoader loader,const std::string &name):mesh(reign::read_god_mesh(loader,name)),pose(mesh){for(auto &c:mesh.cycles)if(folded(c.name)=="default")cycle=&c;}
};
using V3=std::array<float,3>;
V3 add(V3 a,V3 b){return {a[0]+b[0],a[1]+b[1],a[2]+b[2]};}
V3 mul(V3 a,float f){return {a[0]*f,a[1]*f,a[2]*f};}
V3 normalized(V3 a){float n=std::sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);return n>1e-6f?mul(a,1/n):V3{0,0,1};}
std::uint32_t packed(float r,float g,float b,float a=1){auto c=[](float f){return std::uint32_t(std::clamp(std::lround(f*255),0l,255l));};return c(a)<<24|c(r)<<16|c(g)<<8|c(b);}
struct RainString{V3 start,direction;float length,speed;std::vector<float> spacing;};
struct Placement{Model *model;reign::PoseMatrix world;int id,parent=-1;std::string attach;};
}
namespace reign {
struct ShellScene::Impl {
#ifdef REIGN_BINK
 struct MovieTexture {std::unique_ptr<BinkMovie> decoder;unsigned texture=0;std::int64_t frame=-1;float last=-1;};std::map<std::string,MovieTexture> movies;
 ~Impl(){for(auto &entry:movies)if(entry.second.texture)renderer.delete_texture(entry.second.texture);}
#endif
 ResourceStore sources;ConfigLoader loader;LegacyGles &renderer;
 unsigned width=0,height=0;
 std::map<std::string,std::unique_ptr<Model>> models;std::vector<Placement> placements;
 std::unique_ptr<ShellLighting> lighting;std::unique_ptr<Model> sky;
 float farPlane=150,fogDepth=50;bool rainActive=false;float rainBlock=35,rainHeight=49,rainSpeed=10,rainWidth=.08f,rainLength=.76f;unsigned rainGrids=10,rainCount=5;V3 rainDirection{0,-1,0};std::uint32_t rainColor=0xffffffff;std::string rainTexture,rainSound;
 std::vector<RainString> rainStrings;std::vector<V3> rainJitter;
 ShellTerrain terrain;std::unique_ptr<ShellCamera> camera,idle;
 std::unique_ptr<BitmapManager> bitmaps;std::vector<unsigned> textures;
 Impl(const std::string &path,LegacyGles &gpu):sources(path),loader([this](const std::string &name){return sources.load("gamedata",name);}),renderer(gpu){
 for(auto *s:{"mission","world","side"})sources.add_stream("gamedata",s);
 if(!sources.add_archive("gamedata","packs/base.zwp")&&!sources.add_archive("gamedata","base.zwp"))throw std::runtime_error("Missing base archive");
 if(!sources.add_archive("mission","missions/system/shell.zwp")&&!sources.add_archive("mission","shell.zwp"))throw std::runtime_error("Missing shell archive");
 if(!sources.add_archive("world","worlds/sprawl.zwp"))throw std::runtime_error("Missing Sprawl world archive");

 auto game=reign::parse_config(loader,"game.cfg");lighting=std::make_unique<ShellLighting>(game);
 for(auto &n:reign::parse_config(loader,"interface_game_activate.cfg"))if(n.name=="Cmd"){auto text=std::get<std::string>(n.arguments.at(0));if(text.rfind("vid.faroverride ",0)==0)farPlane=std::stof(text.substr(16));if(text.rfind("vid.fogdepth ",0)==0)fogDepth=std::stof(text.substr(13));if(text=="rain.active 1")rainActive=true;}
 if(!std::isfinite(farPlane)||!std::isfinite(fogDepth)||farPlane<=1||farPlane>10000||fogDepth<=0||fogDepth>farPlane)throw std::runtime_error("Invalid shell view distances");
 for(auto &n:game)if(n.name=="Environment")if(auto rain=child(n,"Rain"))for(auto &type:rain->children)if(type.name=="RainType"&&std::get<std::string>(type.arguments.at(0))=="rain"){
  auto setting=[&](const char *name,float fallback){auto n=child(type,name);float value=n?number(n->arguments.at(0)):fallback;if(!std::isfinite(value))throw std::runtime_error("Invalid rain setting");return value;};
  rainBlock=setting("BlockSize",35);rainHeight=rainBlock*1.4f;rainGrids=unsigned(std::clamp(setting("Grids",10),1.f,16.f));rainCount=unsigned(std::clamp(setting("Count",5),1.f,16.f));rainSpeed=setting("Speed",10);rainWidth=setting("Width",.08f);rainLength=setting("Height",.76f);
  if(rainBlock<4||rainBlock>200||rainSpeed<=0||rainSpeed>1000||rainWidth<=0||rainLength<=0)throw std::runtime_error("Invalid rain bounds");
  if(auto d=child(type,"Direction"))for(int i=0;i<3;i++)rainDirection[i]=number(d->arguments.at(i));rainDirection=normalized(rainDirection);
  if(auto c=child(type,"Color"))rainColor=packed(number(c->arguments.at(0))/255,number(c->arguments.at(1))/255,number(c->arguments.at(2))/255,number(c->arguments.at(3))/255);
  if(auto t=child(type,"Texture"))rainTexture=std::get<std::string>(t->arguments.at(0));if(auto a=child(type,"Sound"))rainSound=std::get<std::string>(a->arguments.at(0));
 }
 // Rain::Setup distribution and bead trajectories. A fixed seed makes recovery checks reproducible.
 std::uint32_t seed=0x445232;auto jitter=[&](float x){seed=seed*1664525u+1013904223u;return (float(seed>>8)/16777216.f-.5f)*x;};
 float dx=rainBlock/rainGrids,ds=1.f/rainCount;
 for(unsigned j=0;j<rainGrids*rainGrids;j++){RainString r;r.start={float(j%rainGrids)*dx+jitter(dx)+.5f*dx,rainHeight+jitter(ds*rainHeight*.5f),float(j/rainGrids)*dx+jitter(dx)+.5f*dx};r.direction={rainDirection[0]+jitter(.2f),rainDirection[1]+jitter(.2f),rainDirection[2]+jitter(.2f)};r.length=rainHeight/std::max(.001f,std::abs(r.direction[1]));r.speed=rainSpeed+jitter(rainSpeed*.2f);for(unsigned k=0;k<16;k++)r.spacing.push_back(k*ds+jitter(ds*.5f));rainStrings.push_back(r);}
 for(unsigned k=0;k<rainCount;k++)rainJitter.push_back({jitter(rainDirection[0]*rainBlock*.2f),0,jitter(rainDirection[2]*rainBlock*.2f)});
 for(auto &scope:game)if(scope.name=="WorldInfo")for(auto &c:scope.children){if(c.name=="CellMapX")width=std::get<std::int32_t>(c.arguments.at(0));if(c.name=="CellMapZ")height=std::get<std::int32_t>(c.arguments.at(0));}

 std::map<std::string,std::string> definitions;
 for(const char *file:{"types_objects.cfg","types_world.cfg"})for(auto &type:reign::parse_config(loader,file))if(type.name=="CreateObjectType")if(auto map=child(type,"MapObj"))if(auto god=child(*map,"GodFile")){
  auto id=folded(std::get<std::string>(type.arguments.at(0)));if(!definitions.emplace(id,std::get<std::string>(god->arguments.at(0))).second)throw std::runtime_error("Duplicate model type: "+id);
 }

 for(auto &object:reign::parse_config(loader,"objects.cfg"))if(object.name=="CreateObject")if(auto map=child(object,"MapObj")){
  auto type=folded(std::get<std::string>(object.arguments.at(0)));auto found=definitions.find(type);if(found==definitions.end())throw std::runtime_error("No direct GodFile for shell type: "+type);
  auto name=folded(found->second);auto &model=models[name];if(!model)model=std::make_unique<Model>(loader,name);
  auto parent=child(*map,"Parent");auto position=child(*map,parent?"Orientation":"Position");if(!position)throw std::runtime_error("Shell object has no position/orientation");auto world=reign::identity_pose_matrix;
  for(auto field:std::initializer_list<std::pair<const char*,unsigned>>{{"Right",0},{"Up",4},{"Front",8},{"Pos",12}}){auto vector=child(*position,field.first);if(!vector||vector->arguments.size()!=3)throw std::runtime_error("Invalid shell placement vector");for(unsigned i=0;i<3;++i)world[field.second+i]=number(vector->arguments[i]);}
  Placement placed{model.get(),world,std::get<std::int32_t>(object.arguments.at(1))};if(parent){placed.parent=std::get<std::int32_t>(parent->arguments.at(0));auto point=child(*map,"AttachPoint");if(!point)throw std::runtime_error("Missing attachment point");placed.attach=folded(std::get<std::string>(point->arguments.at(0)));}if(std::any_of(placements.begin(),placements.end(),[&](auto &p){return p.id==placed.id;}))throw std::runtime_error("Duplicate shell object ID");placements.push_back(std::move(placed));
 }
 if(placements.empty())throw std::runtime_error("No shell model placements");

 terrain=reign::load_shell_terrain(loader,width,height);
 for(auto &object:reign::parse_config(loader,"objects.cfg"))for(auto &b:object.children)if(b.name=="BookmarkObj"){auto name=std::get<std::string>(b.arguments.at(0));if(name=="Main::Enter")camera=std::make_unique<reign::ShellCamera>(b);if(name=="Main::Init")idle=std::make_unique<reign::ShellCamera>(b);}
 if(!camera||!idle)throw std::runtime_error("Missing Main::Enter camera");
 // Match the original setup's extra bottom row; recalculated normals/lighting are
 // outside this diagnostic, which displays unlit terrain and posed models.
 terrain.cells.resize((height+1)*terrain.pitch);for(unsigned x=0;x<terrain.pitch;++x){auto &c=terrain.cells[height*terrain.pitch+x];c.height=10;c.color=0xffffffff;}
  bitmaps=std::make_unique<reign::BitmapManager>(loader,[&](const std::string &name){try{loader(name);return true;}catch(const FileSys::FileNotFound&){return false;}},
   reign::BitmapGpuCallbacks{[&](const reign::RgbaImage &image){return renderer.texture_rgba(image.width,image.height,image.pixels.data());},[&](unsigned texture){renderer.delete_texture(texture);},[]{}});
  for(auto &name:terrain.textures)textures.push_back(bitmaps->load(name)->texture);
  sky=std::make_unique<Model>(loader,"engine_sky.god");
  if(rainActive&&!rainTexture.empty())bitmaps->load(rainTexture);


 }
 void draw(float seconds){if(!std::isfinite(seconds)||seconds<0)throw std::invalid_argument("Invalid shell time");
   // Original objectives start Main::Init, then select Main::Enter at 0.5s.
   // Keep the final entrance view after this non-looping bookmark completes.
   auto sample=seconds<0.5f?idle->sample(seconds):camera->sample(std::min(seconds-0.5f,std::nextafter(camera->duration(),0.f)));if(!sample.active)throw std::runtime_error("Camera sample inactive");
   // Shell release configuration supplies far distance and fog depth.
   reign::WorldProjection projection(sample.position,sample.direction,1.0471975512f,float(H)/W,1,farPlane);
   auto light=lighting->sample(seconds);
   auto fog=[&](const std::array<float,4>&p){if(p[3]<=0)return 0xff000000u;float start=(farPlane-fogDepth)*.92f;float minimum=farPlane/(farPlane-1)*(1-1/start);float f=std::max(0.f,(p[2]/p[3]-minimum)/(1-minimum));return packed(0,0,0,std::max(0.f,1-f*f));};
   drawSky(projection,sample.position,light,seconds);
   struct Batch{std::vector<reign::LegacyClip> vertices;std::vector<std::uint16_t> indices;};std::vector<Batch> batches(textures.size());
   for(unsigned cz=0;cz<height;cz+=4)for(unsigned cx=0;cx<width;cx+=4){
    std::array<std::array<float,4>,25> clip{};
    for(unsigned z=0;z<5;++z)for(unsigned x=0;x<5;++x){auto &cell=terrain.cells[(cz+z)*terrain.pitch+cx+x];clip[z*5+x]=projection.transform({float((cx+x)*8)-float(int(terrain.offsetX)),cell.height,float((cz+z)*8)-float(int(terrain.offsetZ))});}
    for(unsigned z=0;z<4;++z)for(unsigned x=0;x<4;++x){auto &cell=terrain.cells[(cz+z)*terrain.pitch+cx+x];if(cell.texture>=batches.size()||cell.uv>44)throw std::runtime_error("Invalid terrain material or UV table offset");auto &batch=batches[cell.texture];
     const unsigned corners[]={z*5+x,(z+1)*5+x,(z+1)*5+x+1,z*5+x+1};
     for(unsigned triangle=0;triangle<6;++triangle){unsigned source=reign::world_detail::clusterI[(z*4+x)*6+triangle],corner=0;while(corner<4&&corners[corner]!=source)++corner;if(corner==4)throw std::runtime_error("Original cluster topology mismatch");auto &uv=reign::world_detail::cellUVList[cell.uv+corner];auto &p=clip[source];auto &litCell=terrain.cells[(cz+source/5)*terrain.pitch+cx+source%5];auto diffuse=ShellLighting::shade(light,terrain.normals.at(litCell.normal),nullptr,litCell.color);
      if(batch.vertices.size()>=65536)throw std::runtime_error("Probe batch exceeds 16-bit indices");batch.indices.push_back(std::uint16_t(batch.vertices.size()));batch.vertices.push_back({p[0],p[1],p[2],p[3],diffuse,fog(p),uv.u,uv.v});
     }
    }
   }
   reign::DrawState state;state.depth_test=true;state.depth_write=true;state.filtered=true;state.texture_op=reign::TextureOp::modulate_rgba;state.fog=true;for(int i=0;i<3;i++)state.fog_rgb[i]=light.fog[i];
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
    for(auto &face:mesh.faces){auto &batch=modelBatches.at(face.bucket);for(unsigned i=0;i<3;++i){auto p=projection.transform(pose.vertices.at(face.vertices[i]));auto uv=mesh.uvs.at(face.uvs[i]);auto normal=mesh.normals.at(face.normals[i]);auto &skin=pose.skinMatrices.at(mesh.vertexWeights.at(face.vertices[i]).at(0).state);V3 transformed{};for(unsigned c=0;c<3;c++)transformed[c]=normal[0]*skin[c]+normal[1]*skin[c+4]+normal[2]*skin[c+8];auto &bucket=mesh.buckets.at(face.bucket);auto base=mesh.colors.empty()?0xffffffff:mesh.colors.at(face.vertices[i]);auto diffuse=((bucket.flags>>28)&7)==1?base:ShellLighting::shade(light,transformed,bucket.material?&*bucket.material:nullptr,base);if(batch.vertices.size()>=65536)throw std::runtime_error("Model batch too large");batch.indices.push_back(std::uint16_t(batch.vertices.size()));batch.vertices.push_back({p[0],p[1],p[2],p[3],diffuse,fog(p),uv[0],uv[1]});}}
    for(unsigned i=0;i<modelBatches.size();++i){auto &batch=modelBatches[i];if(batch.indices.empty())continue;auto &texture=mesh.buckets[i].texture0;state.texture=texture&&!texture->name.empty()?animatedTexture(texture->name,seconds):0;state.texture_op=state.texture?reign::TextureOp::modulate_rgba:reign::TextureOp::diffuse;
     auto flags=mesh.buckets[i].flags;unsigned op=(flags>>28)&7,src=(flags>>16)&15,dst=(flags>>20)&15;
     if(state.texture){if(op==1)state.texture_op=TextureOp::texture;else if(op==2)state.texture_op=TextureOp::decal;else if(op==3)state.texture_op=TextureOp::modulate_rgb_texture_alpha;else if(op==4)state.texture_op=TextureOp::modulate2x;else if(op==5)state.texture_op=TextureOp::modulate4x;}
     state.blend=Blend::opaque;if(src>=1&&src<=11&&dst>=1&&dst<=10){state.blend=Blend::custom;state.source_factor=static_cast<BlendFactor>(src);state.destination_factor=static_cast<BlendFactor>(dst);}
     renderer.draw_clip(W,H,batch.vertices.data(),batch.vertices.size(),batch.indices.data(),batch.indices.size(),state);modelTriangles+=batch.indices.size()/3;}
   }
   drawRain(projection,sample.position,sample.direction,seconds);
 }
 unsigned animatedTexture(const std::string &name,float seconds){
#ifdef REIGN_BINK
  auto stem=name.substr(0,name.find_last_of('.'));if(stem=="movie-billboard"){
   auto &m=movies[stem];if(!m.decoder)m.decoder=std::make_unique<BinkMovie>(loader(stem+".bik"));float t=std::fmod(seconds,float(m.decoder->duration()));if(t<m.last){m.decoder=std::make_unique<BinkMovie>(loader(stem+".bik"));m.frame=-1;}m.last=t;m.decoder->advance(t);m.decoder->drain_audio();if(m.frame!=m.decoder->frame_index()){if(m.texture)renderer.delete_texture(m.texture);m.texture=renderer.texture_rgba(m.decoder->width(),m.decoder->height(),m.decoder->rgba().data());m.frame=m.decoder->frame_index();}return m.texture;
  }
#endif
 auto first=bitmaps->load(name);auto frames=bitmaps->animation(name);return frames.empty()?first->texture:frames[std::size_t(std::floor(seconds*std::max(0.f,terrain.environment.animationRate)))%frames.size()]->texture;}
 void drawSky(const WorldProjection &projection,V3 camera,const ShellLightSample &light,float seconds){
  auto world=identity_pose_matrix;world[0]=world[5]=world[10]=farPlane;world[12]=camera[0];world[13]=camera[1]+(100-camera[1])*.4f+terrain.environment.skyLevel;world[14]=camera[2];
  auto pose=sky->pose.sample("",0,world);auto &g=sky->mesh.geometry;
  for(unsigned b=0;b<g.buckets.size();b++){std::vector<LegacyClip> vertices;std::vector<std::uint16_t> indices;auto &bucket=g.buckets[b];
   for(auto &face:g.faces)if(face.bucket==b)for(unsigned i=0;i<3;i++){auto index=face.vertices[i];auto p=projection.transform(pose.vertices[index]);auto uv=g.uvs.at(face.uvs[i]);float low=g.offset[1]-g.extent[1],height=g.extent[1]*2*.7f;float alpha=height>0?std::clamp((g.vertices[index][1]-low)/height,0.f,1.f):1.f;if(vertices.size()>=65536)throw std::runtime_error("Sky batch overflow");indices.push_back(vertices.size());vertices.push_back({p[0],p[1],p[2],p[3],packed(light.sun[0],light.sun[1],light.sun[2],alpha),0xff000000,uv[0],uv[1]});}
   DrawState state;state.blend=Blend::alpha;state.filtered=true;state.texture_op=TextureOp::modulate_rgba;if(bucket.texture0)state.texture=animatedTexture(bucket.texture0->name,seconds);renderer.draw_clip(W,H,vertices.data(),vertices.size(),indices.data(),indices.size(),state);
  }
  if(terrain.environment.showClouds){std::vector<LegacyClip> v;std::vector<std::uint16_t> indices;float wind=std::fmod(seconds*terrain.environment.windSpeed,1.f);for(unsigned z=0;z<=8;z++)for(unsigned x=0;x<=8;x++){auto p=projection.transform({camera[0]+farPlane*(-1.5f+3.f*x/8),world[13]+terrain.environment.cloudOffset,camera[2]+farPlane*(-1.5f+3.f*z/8)});v.push_back({p[0],p[1],p[2],p[3],0xffffffff,0xff000000,-.5f+3.f*x/8,-.5f+3.f*z/8+wind});if(x&&z){unsigned at=z*9+x;for(unsigned n:{at,at-1,at-10,at,at-10,at-9})indices.push_back(n);}}DrawState state;state.blend=Blend::additive;state.filtered=true;state.address=TextureAddress::wrap;state.texture_op=TextureOp::modulate_rgba;state.texture=animatedTexture("engine_cloud.pic",seconds);renderer.draw_clip(W,H,v.data(),v.size(),indices.data(),indices.size(),state);}
 }
 void drawRain(const WorldProjection &projection,V3 camera,V3 front,float seconds){
  if(!rainActive||rainTexture.empty())return;
  V3 origin{rainBlock*.5f*std::floor(2*camera[0]/rainBlock)-rainDirection[0]*(rainBlock+(rainHeight-rainBlock)*.4f),rainHeight*.5f*std::floor(2*camera[1]/rainHeight)-(rainHeight-rainBlock),rainBlock*.5f*std::floor(2*camera[2]/rainBlock)-rainDirection[2]*(rainBlock+(rainHeight-rainBlock)*.4f)};
  auto side=normalized(V3{front[2],0,-front[0]});std::vector<LegacyClip> vertices;std::vector<std::uint16_t> indices;
  for(int z=-1;z<=1;z++)for(int x=-1;x<=1;x++)for(auto &r:rainStrings)for(unsigned k=0;k<rainCount;k++){
   float t=r.length*std::fmod(seconds*r.speed/rainHeight+r.spacing[k]+1,1.f);auto center=add(add(add(origin,{x*rainBlock,0,z*rainBlock}),r.start),add(mul(r.direction,t),rainJitter[k]));
   auto end=add(center,mul(rainDirection,rainLength));V3 points[]={center,add(end,mul(side,rainWidth*.5f)),add(end,mul(side,-rainWidth*.5f))};
   for(unsigned i=0;i<3;i++){auto p=projection.transform(points[i]);indices.push_back(vertices.size());vertices.push_back({p[0],p[1],p[2],p[3],rainColor,0xff000000,i==0?.5f:i==1?0.f:1.f,i==0?0.f:1.f});}
  }
  DrawState state;state.blend=Blend::alpha;state.depth_test=true;state.depth_write=false;state.filtered=true;state.texture_op=TextureOp::modulate_rgba;state.texture=animatedTexture(rainTexture,seconds);renderer.draw_clip(W,H,vertices.data(),vertices.size(),indices.data(),indices.size(),state);
 }
};
ShellScene::ShellScene(const std::string &path,LegacyGles &gpu):impl_(std::make_unique<Impl>(path,gpu)){}
ShellScene::~ShellScene()=default;
void ShellScene::draw(float seconds){impl_->draw(seconds);}
void ShellScene::abandon_context(){impl_->bitmaps->abandon_context();
#ifdef REIGN_BINK
 for(auto &entry:impl_->movies){entry.second.texture=0;entry.second.frame=-1;}
#endif
}
std::string ShellScene::ambient_sound()const{return impl_->rainActive?impl_->rainSound:std::string{};}
void ShellScene::finish_frame(float seconds){
 // Original StepHoldPrim Time(0,1,4), Direction("up"), black fade after IFace.
 float alpha=seconds<1?1:seconds<4?(4-seconds)/3:0;if(alpha<=0)return;
 LegacyTL v[]={{0,0,0,1,packed(0,0,0,alpha),0xff000000,0,0},{640,0,0,1,packed(0,0,0,alpha),0xff000000,0,0},{640,480,0,1,packed(0,0,0,alpha),0xff000000,0,0},{0,480,0,1,packed(0,0,0,alpha),0xff000000,0,0}};std::uint16_t indices[]={0,1,2,0,2,3};DrawState state;state.blend=Blend::alpha;impl_->renderer.draw(640,480,v,4,indices,6,state);
}
}
