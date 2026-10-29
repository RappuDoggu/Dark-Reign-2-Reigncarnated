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
#include <set>
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
float setting(const reign::ConfigNode &n,const char *key,float fallback){auto c=child(n,key);float v=c?number(c->arguments.at(0)):fallback;if(!std::isfinite(v))throw std::runtime_error("Invalid effect setting");return v;}
std::string textSetting(const reign::ConfigNode &n,const char *key,std::string fallback={}){auto c=child(n,key);return c?std::get<std::string>(c->arguments.at(0)):fallback;}
V3 vectorSetting(const reign::ConfigNode &n,const char *key,V3 fallback={}){auto c=child(n,key);if(c)for(int i=0;i<3;i++)fallback[i]=number(c->arguments.at(i));return fallback;}
float randomUnit(std::uint32_t seed){seed^=seed>>16;seed*=0x7feb352d;seed^=seed>>15;seed*=0x846ca68b;seed^=seed>>16;return float(seed>>8)/16777216.f;}
struct ParticleDefinition{std::string texture,simulation;float life=1,animation=1;bool persistent=false,additive=false;std::vector<std::array<float,5>> colors;std::vector<std::array<float,2>> radii;};
struct Emitter{std::string particle,point;V3 velocity{},random{};unsigned count=1;float delay=.1f;};
struct SplatDefinition{bool water=false;unsigned count=0;float life=.2f,minWidth=.08f,maxWidth=.5f,minHeight=.02f,maxHeight=1,offset=0;std::uint32_t color=0xffffffff;std::string texture;};
struct RainString{V3 start,direction;float length,speed;std::vector<float> spacing;};
struct Placement{Model *model;reign::PoseMatrix world;int id,parent=-1;std::string attach,type;};
}
namespace reign {
struct ShellScene::Impl {
#ifdef REIGN_BINK
 struct MovieTexture {std::unique_ptr<BinkMovie> decoder;unsigned texture=0;std::int64_t frame=-1;float last=-1;};std::map<std::string,MovieTexture> movies;
 ~Impl(){for(auto &entry:movies)if(entry.second.texture)renderer.delete_texture(entry.second.texture);}
#endif
 ResourceStore sources;ConfigLoader loader;LegacyGles &renderer;
 ShellEffectStats stats;float strikeChance=0;std::int64_t lastStrike=-1;std::vector<std::string> sounds;
 unsigned width=0,height=0;
 std::map<std::string,bool> textureAlpha;
 std::map<std::string,std::unique_ptr<Model>> models;std::vector<Placement> placements;
 std::unique_ptr<ShellLighting> lighting;std::unique_ptr<Model> sky;
 float farPlane=150,fogDepth=50;bool rainActive=false;float rainBlock=35,rainHeight=49,rainSpeed=10,rainWidth=.08f,rainLength=.76f;unsigned rainGrids=10,rainCount=5;V3 rainDirection{0,-1,0};std::uint32_t rainColor=0xffffffff;std::string rainTexture,rainSound;
 std::map<std::string,std::vector<Emitter>> emitters;std::map<std::string,ParticleDefinition> particles;std::vector<SplatDefinition> splats;
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
  strikeChance=setting("ChanceOfStrike",0);rainBlock=setting("BlockSize",35);rainHeight=rainBlock*1.4f;rainGrids=unsigned(std::clamp(setting("Grids",10),1.f,16.f));rainCount=unsigned(std::clamp(setting("Count",5),1.f,16.f));rainSpeed=setting("Speed",10);rainWidth=setting("Width",.08f);rainLength=setting("Height",.76f);
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

 for(auto &n:game)if(n.name=="Environment")if(auto rain=child(n,"Rain"))for(auto &t:rain->children)if(t.name=="SplatType"){
  SplatDefinition d;d.water=textSetting(t,"Type")=="Water";d.count=unsigned(std::clamp(setting(t,"Count",0),0.f,256.f));d.life=setting(t,"LifeSpan",.2f);d.minWidth=setting(t,"MinWidth",.08f);d.maxWidth=setting(t,"MaxWidth",.5f);d.minHeight=setting(t,"MinHeight",.02f);d.maxHeight=setting(t,"MaxHeight",1);d.offset=setting(t,"Offset",0);d.texture=textSetting(t,"Texture");if(auto c=child(t,"Color"))d.color=packed(number(c->arguments.at(0))/255,number(c->arguments.at(1))/255,number(c->arguments.at(2))/255,number(c->arguments.at(3))/255);if(d.life<=0||d.life>10)throw std::runtime_error("Invalid splat lifetime");splats.push_back(d);
 }
 std::set<std::string> usedTypes;for(auto &o:parse_config(loader,"objects.cfg"))if(o.name=="CreateObject"&&!o.arguments.empty())usedTypes.insert(folded(std::get<std::string>(o.arguments[0])));
 std::vector<ConfigNode> types;for(auto file:{"types_objects.cfg","types_world.cfg","types_particles.cfg"}){auto nodes=parse_config(loader,file);types.insert(types.end(),nodes.begin(),nodes.end());}
 std::map<std::string,const ConfigNode*> effects,renders,particleTypes;
 for(auto &n:types)if(!n.arguments.empty()){auto name=folded(std::get<std::string>(n.arguments[0]));if(n.name=="CreateEffectType")effects[name]=&n;if(n.name=="CreateParticleType")particleTypes[name]=&n;if(n.name=="CreateParticleRenderType")renders[name]=&n;}
 for(auto &n:types)if(n.name=="CreateObjectType"&&usedTypes.count(folded(std::get<std::string>(n.arguments.at(0)))))if(auto m=child(n,"MapObj"))if(auto fx=child(*m,"GenericFX"))for(auto &a:fx->children)if(a.name=="Add"&&std::get<std::string>(a.arguments.at(0))=="MapObj::Smoke"){
  auto found=effects.find(folded(std::get<std::string>(a.arguments.at(1))));if(found==effects.end())continue;auto list=child(*found->second,"Particles");if(!list)continue;
  for(auto &e:list->children)if(e.name=="Add"){Emitter out;out.particle=folded(textSetting(e,"Particle"));out.point=folded(textSetting(e,"Point"));out.count=unsigned(std::clamp(e.arguments.empty()?1.f:number(e.arguments.at(0)),1.f,64.f));out.delay=std::max(.1f,setting(e,"Delay",.1f));out.velocity=vectorSetting(e,"VelocityPosition");out.random=vectorSetting(e,"Random");emitters[folded(std::get<std::string>(n.arguments[0]))].push_back(out);}
 }
 std::set<std::string> neededParticles;for(auto &group:emitters)for(auto &emitter:group.second)neededParticles.insert(emitter.particle);
 std::map<std::string,std::vector<std::string>> particleLayers;
 for(auto &entry:particleTypes){if(!neededParticles.count(entry.first))continue;auto &n=*entry.second;unsigned layer=0;
  for(auto &base:n.children)if(base.name=="RenderBase"){auto r=renders.find(folded(std::get<std::string>(base.arguments.at(0))));if(r==renders.end())throw std::runtime_error("Missing particle renderer");ParticleDefinition d;d.life=setting(n,"LifeTime",setting(n,"lifeTime",1));d.persistent=d.life==0;if(d.persistent)d.life=setting(*r->second,"LifeTime",1);d.simulation=std::get<std::string>(n.arguments.at(1));d.texture=textSetting(*r->second,"TextureName");d.animation=setting(*r->second,"AnimateTime",1);d.additive=textSetting(*r->second,"TextureBlend").find("srcalpha one ")!=std::string::npos;if(d.life<=0||d.life>60)throw std::runtime_error("Invalid shell particle lifetime: "+entry.first);
   for(auto &k:r->second->children){if(k.name=="ColorKey"){std::array<float,5> c{};for(int i=0;i<5;i++)c[i]=number(k.arguments.at(i));d.colors.push_back(c);}if(k.name=="StartColor"||k.name=="FinishColor"){std::array<float,5> c{k.name=="StartColor"?0.f:1.f,0,0,0,0};for(int i=0;i<4;i++)c[i+1]=number(k.arguments.at(i));d.colors.push_back(c);}if(k.name=="StartRadius"||k.name=="FinishRadius")d.radii.push_back({k.name=="StartRadius"?0.f:1.f,number(k.arguments.at(0))});if(k.name=="RadiusKey"||k.name=="ScaleKey")d.radii.push_back({number(k.arguments.at(0)),number(k.arguments.at(1))});}
   if(d.colors.empty()||d.radii.empty())throw std::runtime_error("Missing shell particle keys: "+entry.first);auto key=entry.first+"#"+std::to_string(layer++);particles[key]=d;particleLayers[entry.first].push_back(key);
  }
 }
 for(auto &group:emitters){std::vector<Emitter> layers;for(auto &e:group.second){auto found=particleLayers.find(e.particle);if(found==particleLayers.end())throw std::runtime_error("Unsupported shell particle: "+e.particle);for(auto &key:found->second){auto copy=e;copy.particle=key;layers.push_back(copy);}}group.second=std::move(layers);}
 std::map<std::string,std::string> definitions;
 for(const char *file:{"types_objects.cfg","types_world.cfg"})for(auto &type:reign::parse_config(loader,file))if(type.name=="CreateObjectType")if(auto map=child(type,"MapObj"))if(auto god=child(*map,"GodFile")){
  auto id=folded(std::get<std::string>(type.arguments.at(0)));if(!definitions.emplace(id,std::get<std::string>(god->arguments.at(0))).second)throw std::runtime_error("Duplicate model type: "+id);
 }

 for(auto &object:reign::parse_config(loader,"objects.cfg"))if(object.name=="CreateObject")if(auto map=child(object,"MapObj")){
  auto type=folded(std::get<std::string>(object.arguments.at(0)));auto found=definitions.find(type);if(found==definitions.end())throw std::runtime_error("No direct GodFile for shell type: "+type);
  auto name=folded(found->second);auto &model=models[name];if(!model)model=std::make_unique<Model>(loader,name);
  auto parent=child(*map,"Parent");auto position=child(*map,parent?"Orientation":"Position");if(!position)throw std::runtime_error("Shell object has no position/orientation");auto world=reign::identity_pose_matrix;
  for(auto field:std::initializer_list<std::pair<const char*,unsigned>>{{"Right",0},{"Up",4},{"Front",8},{"Pos",12}}){auto vector=child(*position,field.first);if(!vector||vector->arguments.size()!=3)throw std::runtime_error("Invalid shell placement vector");for(unsigned i=0;i<3;++i)world[field.second+i]=number(vector->arguments[i]);}
  Placement placed{model.get(),world,std::get<std::int32_t>(object.arguments.at(1))};placed.type=type;if(parent){placed.parent=std::get<std::int32_t>(parent->arguments.at(0));auto point=child(*map,"AttachPoint");if(!point)throw std::runtime_error("Missing attachment point");placed.attach=folded(std::get<std::string>(point->arguments.at(0)));}if(std::any_of(placements.begin(),placements.end(),[&](auto &p){return p.id==placed.id;}))throw std::runtime_error("Duplicate shell object ID");placements.push_back(std::move(placed));
 }
 if(placements.empty())throw std::runtime_error("No shell model placements");

 terrain=reign::load_shell_terrain(loader,width,height);
 for(auto &object:reign::parse_config(loader,"objects.cfg"))for(auto &b:object.children)if(b.name=="BookmarkObj"){auto name=std::get<std::string>(b.arguments.at(0));if(name=="Main::Enter")camera=std::make_unique<reign::ShellCamera>(b);if(name=="Main::Init")idle=std::make_unique<reign::ShellCamera>(b);}
 if(!camera||!idle)throw std::runtime_error("Missing Main::Enter camera");
 // Match the original setup's extra bottom row.
 terrain.cells.resize((height+1)*terrain.pitch);for(unsigned x=0;x<terrain.pitch;++x){auto &c=terrain.cells[height*terrain.pitch+x];c.height=10;c.color=0xffffffff;}
  bitmaps=std::make_unique<reign::BitmapManager>(loader,[&](const std::string &name){try{loader(name);return true;}catch(const FileSys::FileNotFound&){return false;}},
   reign::BitmapGpuCallbacks{[&](const reign::RgbaImage &image){return renderer.texture_rgba(image.width,image.height,image.pixels.data());},[&](unsigned texture){renderer.delete_texture(texture);},[]{}});
  for(auto &name:terrain.textures)textures.push_back(bitmaps->load(name)->texture);
  sky=std::make_unique<Model>(loader,"engine_sky.god");
  if(rainActive&&!rainTexture.empty())bitmaps->load(rainTexture);


 }
 void draw(float seconds){stats={};if(!std::isfinite(seconds)||seconds<0)throw std::invalid_argument("Invalid shell time");
   // Original objectives start Main::Init, then select Main::Enter at 0.5s.
   // Keep the final entrance view after this non-looping bookmark completes.
   auto sample=seconds<0.5f?idle->sample(seconds):camera->sample(std::min(seconds-0.5f,std::nextafter(camera->duration(),0.f)));if(!sample.active)throw std::runtime_error("Camera sample inactive");
   // Shell release configuration supplies far distance and fog depth.
   reign::WorldProjection projection(sample.position,sample.direction,1.0471975512f,float(H)/W,1,farPlane);
   auto light=lighting->sample(seconds);
   // Weather advances on the original 10 Hz simulation cadence, independent of GPU FPS.
   auto weatherTick=std::int64_t(seconds*10);for(auto tick=std::max<std::int64_t>(0,weatherTick-6);rainActive&&tick<weatherTick;tick++)if(randomUnit(std::uint32_t(tick)^0x57ea123u)<=strikeChance){auto duration=2+unsigned(randomUnit(std::uint32_t(tick)^0x6789123u)*5);if(weatherTick-tick<=duration){float flash=.2f+randomUnit(std::uint32_t(weatherTick)^0x32f976u);light.ambient={flash,flash,flash*1.5f};if(tick>lastStrike){sounds.push_back("engine_thunder.wav");lastStrike=tick;}break;}}

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
   struct TransparentBatch{Batch batch;DrawState state;float depth;};std::vector<TransparentBatch> transparent;
   unsigned modelTriangles=0;
   for(std::size_t object=0;object<placements.size();++object){auto &model=*placements[object].model;auto &pose=poses[object];auto &mesh=model.mesh.geometry;
    std::vector<Batch> modelBatches(mesh.buckets.size());
    for(auto &face:mesh.faces){auto &batch=modelBatches.at(face.bucket);for(unsigned i=0;i<3;++i){auto p=projection.transform(pose.vertices.at(face.vertices[i]));auto uv=mesh.uvs.at(face.uvs[i]);auto normal=mesh.normals.at(face.normals[i]);auto &skin=pose.skinMatrices.at(mesh.vertexWeights.at(face.vertices[i]).at(0).state);V3 transformed{};for(unsigned c=0;c<3;c++)transformed[c]=normal[0]*skin[c]+normal[1]*skin[c+4]+normal[2]*skin[c+8];auto &bucket=mesh.buckets.at(face.bucket);auto base=mesh.colors.empty()?0xffffffff:mesh.colors.at(face.vertices[i]);auto diffuse=((bucket.flags>>28)&7)==1?base:ShellLighting::shade(light,transformed,bucket.material?&*bucket.material:nullptr,base);if(batch.vertices.size()>=65536)throw std::runtime_error("Model batch too large");batch.indices.push_back(std::uint16_t(batch.vertices.size()));batch.vertices.push_back({p[0],p[1],p[2],p[3],diffuse,fog(p),uv[0],uv[1]});}}
    for(unsigned i=0;i<modelBatches.size();++i){auto &batch=modelBatches[i];if(batch.indices.empty())continue;auto &texture=mesh.buckets[i].texture0;state.texture=texture&&!texture->name.empty()?animatedTexture(texture->name,seconds):0;state.texture_op=state.texture?reign::TextureOp::modulate_rgba:reign::TextureOp::diffuse;
     auto flags=mesh.buckets[i].flags;unsigned op=(flags>>28)&7,src=(flags>>16)&15,dst=(flags>>20)&15;
     if(state.texture){if(op==1)state.texture_op=TextureOp::texture;else if(op==2)state.texture_op=TextureOp::decal;else if(op==3)state.texture_op=TextureOp::modulate_rgb_texture_alpha;else if(op==4)state.texture_op=TextureOp::modulate2x;else if(op==5)state.texture_op=TextureOp::modulate4x;}
     state.blend=Blend::opaque;if(src>=1&&src<=11&&dst>=1&&dst<=10){state.blend=Blend::custom;state.source_factor=static_cast<BlendFactor>(src);state.destination_factor=static_cast<BlendFactor>(dst);}
     bool hasAlpha=false;if(texture&&!texture->name.empty()&&texture->name.rfind("movie-billboard",0)!=0){auto cached=textureAlpha.find(texture->name);if(cached==textureAlpha.end()){auto bitmap=bitmaps->load(texture->name);bool alpha=false;for(std::size_t a=3;a<bitmap->image.pixels.size();a+=4)if(bitmap->image.pixels[a]<255){alpha=true;break;}cached=textureAlpha.emplace(texture->name,alpha).first;}hasAlpha=cached->second;}auto &material=mesh.buckets[i].material;bool alphaBlend=state.source_factor==BlendFactor::source_alpha&&state.destination_factor==BlendFactor::inverse_source_alpha;bool translucent=(hasAlpha||(material&&material->diffuse[3]<1))||(state.blend==Blend::custom&&!alphaBlend&&(state.source_factor!=BlendFactor::one||state.destination_factor!=BlendFactor::zero));state.depth_write=!translucent;modelTriangles+=batch.indices.size()/3;
     if(translucent){float depth=0;for(auto &v:batch.vertices)depth+=v.w;depth/=batch.vertices.size();transparent.push_back({std::move(batch),state,depth});}else renderer.draw_clip(W,H,batch.vertices.data(),batch.vertices.size(),batch.indices.data(),batch.indices.size(),state);}
   }
   drawWater(projection,light,seconds);
   std::stable_sort(transparent.begin(),transparent.end(),[](auto &a,auto &b){return a.depth>b.depth;});for(auto &b:transparent)renderer.draw_clip(W,H,b.batch.vertices.data(),b.batch.vertices.size(),b.batch.indices.data(),b.batch.indices.size(),b.state);
   drawParticles(projection,sample.direction,poses,seconds);
   drawSplats(projection,sample.position,sample.direction,seconds);
   drawRain(projection,sample.position,sample.direction,seconds);
 }
 // Terrain queries share the original diagonal split and cluster water bit.
 bool surface(float x,float z,float &ground,float &water)const{
  float gx=(x+terrain.offsetX)/8,gz=(z+terrain.offsetZ)/8;if(gx<0||gz<0||gx>=width||gz>=height)return false;unsigned ix=unsigned(gx),iz=unsigned(gz);float u=gx-ix,v=gz-iz;
  float a=terrain.cells[iz*terrain.pitch+ix].height,b=terrain.cells[(iz+1)*terrain.pitch+ix].height,c=terrain.cells[(iz+1)*terrain.pitch+ix+1].height,d=terrain.cells[iz*terrain.pitch+ix+1].height;
  ground=u<v?a+(b-a)*v+(c-b)*u:a+(d-a)*u+(c-d)*v;auto &cluster=terrain.clusters.at((iz/4)*(width/4)+ix/4);water=(cluster.status&1)?cluster.waterHeight:-1e10f;return true;
 }
 void quad(const WorldProjection &projection,V3 center,V3 right,V3 up,std::uint32_t color,unsigned texture,bool additive=false){
  V3 points[]={add(add(center,mul(right,-1)),up),add(add(center,right),up),add(add(center,right),mul(up,-1)),add(add(center,mul(right,-1)),mul(up,-1))};LegacyClip v[4];
  for(unsigned i=0;i<4;i++){auto p=projection.transform(points[i]);v[i]={p[0],p[1],p[2],p[3],color,0xff000000,i==1||i==2?1.f:0.f,i>=2?1.f:0.f};}std::uint16_t ix[]={0,1,2,0,2,3};DrawState state;state.depth_test=true;state.depth_write=false;state.blend=additive?Blend::additive:Blend::alpha;state.filtered=true;state.texture_op=TextureOp::modulate_rgba;state.texture=texture;renderer.draw_clip(W,H,v,4,ix,6,state);
 }
 void drawWater(const WorldProjection &projection,const ShellLightSample &light,float seconds){
  bool any=std::any_of(terrain.clusters.begin(),terrain.clusters.end(),[](auto &c){return c.status&1;});if(!any)return;
  auto &env=terrain.environment;float wave=std::cos(seconds*env.waveSpeed)*env.waveHeight,scroll=std::fmod(seconds*env.waterSpeed,1.f);DrawState state;state.texture=animatedTexture("engine_water.tga",seconds);state.depth_test=true;state.depth_write=false;state.blend=Blend::alpha;state.filtered=true;state.address=TextureAddress::wrap;state.texture_op=TextureOp::modulate_rgba;
  for(unsigned cz=0;cz<height/4;cz++)for(unsigned cx=0;cx<width/4;cx++){auto &cluster=terrain.clusters.at(cz*(width/4)+cx);if(!(cluster.status&1))continue;stats.waterClusters++;
   for(unsigned layer=0;layer<1;layer++){LegacyClip v[25];float alpha=env.waterAlphaBottom/255.f;if(layer)alpha*=env.waterAlphaTopFactor;auto color=ShellLighting::shade(light,{0,1,0},nullptr,(std::uint32_t(std::clamp(alpha,0.f,1.f)*255)<<24)|(env.waterColorBottom&0xffffff));
    for(unsigned z=0;z<5;z++)for(unsigned x=0;x<5;x++){float heightWave=wave*((cx+cz)%2?-1:1)*(1-z*.5f)*(1-x*.5f);auto p=projection.transform({float(cx*32+x*8)-terrain.offsetX,cluster.waterHeight+heightWave,float(cz*32+z*8)-terrain.offsetZ});float u=x*.25f,w=z*.25f;if(layer){u=1-u;u-=scroll;w-=scroll;}else{u+=scroll;w+=scroll;}v[z*5+x]={p[0],p[1],p[2],p[3],color,0xff000000,u,w};}
    renderer.draw_clip(W,H,v,25,world_detail::clusterI,96,state);
   }
  }
 }
 void drawParticles(const WorldProjection &projection,V3 front,const std::vector<GodPoseFrame>&poses,float seconds){
  auto right=normalized(V3{front[2],0,-front[0]});auto up=normalized(V3{front[1]*right[2]-front[2]*right[1],front[2]*right[0]-front[0]*right[2],front[0]*right[1]-front[1]*right[0]});
  struct Sprite{V3 center;float radius,depth;std::uint32_t color;unsigned texture;bool additive;};std::vector<Sprite> sprites;
  for(unsigned oi=0;oi<placements.size();oi++){auto &placed=placements[oi];auto found=emitters.find(placed.type);if(found==emitters.end())continue;for(auto &e:found->second){stats.emitters++;auto p=particles.find(e.particle);if(p==particles.end())throw std::runtime_error("Unsupported shell particle: "+e.particle);auto &d=p->second;if(d.simulation!="Smoke"&&d.simulation!="Ember"&&d.simulation!="Particle")throw std::runtime_error("Unsupported shell particle simulator: "+d.simulation);
   auto &nodes=placed.model->mesh.nodes;auto node=std::find_if(nodes.begin(),nodes.end(),[&](auto &n){return folded(n.name)==e.point;});if(node==nodes.end())throw std::runtime_error("Missing effect attachment: "+e.point);auto &matrix=poses[oi].worldMatrices.at(node->state);V3 origin{matrix[12],matrix[13],matrix[14]};
   auto end=d.persistent?0:std::uint64_t(seconds/e.delay);auto start=d.persistent?0:std::uint64_t(std::max(0.f,seconds-d.life)/e.delay);if(end-start>6000)throw std::runtime_error("Particle budget exceeded");
   for(auto tick=start;tick<=end;tick++)for(unsigned k=0;k<e.count;k++){float age=seconds-float(tick)*e.delay;if(age<0||(!d.persistent&&age>=d.life))continue;auto seed=std::uint32_t(tick)*131u+placed.id*7919u+k*31u;V3 velocity=e.velocity;for(int c=0;c<3;c++)velocity[c]*=1-2*e.random[c]*randomUnit(seed+c*65537u);V3 pos=origin;
    // Original Ember integrates gravity before movement and reflects on terrain.
    float left=d.persistent?0:age;while(left>0){float dt=std::min(.1f,left);if(d.simulation=="Ember")velocity[1]-=9.8f*dt;pos=add(pos,mul(velocity,dt));if(d.simulation=="Ember"){float ground,water;if(!surface(pos[0],pos[2],ground,water)){pos[1]=-1e10f;break;}float floor=std::max(ground,water);if(pos[1]<=floor){pos[1]=floor;velocity[0]*=.5f;velocity[1]*=-.5f;velocity[2]*=.5f;}}left-=dt;}
    float life=std::min(1.f,age/d.life*d.animation);auto interpolate=[&](const auto &keys,unsigned column){if(life<=keys.front()[0])return keys.front()[column];for(std::size_t i=1;i<keys.size();i++)if(life<=keys[i][0]){float t=(life-keys[i-1][0])/(keys[i][0]-keys[i-1][0]);return keys[i-1][column]+t*(keys[i][column]-keys[i-1][column]);}return keys.back()[column];};float radius=interpolate(d.radii,1);auto color=packed(interpolate(d.colors,1)/255,interpolate(d.colors,2)/255,interpolate(d.colors,3)/255,interpolate(d.colors,4)/255);auto texture=bitmaps->load(d.texture)->texture;auto frames=bitmaps->animation(d.texture);if(d.animation>0&&!frames.empty())texture=frames[std::min(std::size_t(age*d.animation*10),frames.size()-1)]->texture;auto clip=projection.transform(pos);if(clip[3]>0)sprites.push_back({pos,radius,clip[3],color,texture,d.additive});
   }
  }}
  std::stable_sort(sprites.begin(),sprites.end(),[](auto&a,auto&b){return a.depth>b.depth;});stats.particles=sprites.size();for(auto &s:sprites)quad(projection,s.center,mul(right,s.radius),mul(up,s.radius),s.color,s.texture,s.additive);
 }
 void drawSplats(const WorldProjection &projection,V3 camera,V3 front,float seconds){
  if(!rainActive)return;float cg,cw;if(!surface(camera[0],camera[2],cg,cw))return;float dy=std::max(0.f,camera[1]-std::max(cg,cw)),block=rainBlock+dy;auto side=normalized(V3{front[2],0,-front[0]});
  for(auto &d:splats){unsigned n=unsigned(std::sqrt(float(d.count)));if(!n||d.texture.empty())continue;float spacing=block/n;auto texture=animatedTexture(d.texture,seconds);float x0=camera[0]+front[0]*(1+dy*d.offset*rainBlock*.6f)-block*.5f,z0=camera[2]+front[2]*(1+dy*d.offset*rainBlock*.6f)-block*.5f;
   for(unsigned i=0;i<n*n;i++){float period=d.life*(1+randomUnit(i+237))+randomUnit(i+411)*.25f;float shifted=seconds+randomUnit(i+673)*period;auto cycle=std::uint32_t(shifted/period);float age=std::fmod(shifted,period),life=d.life*(1+randomUnit(i+237));if(age>=life)continue;auto seed=cycle*7919u+i*131u;float x=x0+(i%n+randomUnit(seed))*spacing,z=z0+(i/n+randomUnit(seed+67))*spacing,g,w;if(!surface(x,z,g,w))continue;bool water=w>=g;if(water!=d.water)continue;if(water)stats.waterSplats++;else stats.groundSplats++;float t=age/life,width=d.minWidth+(d.maxWidth-d.minWidth)*t,height=d.minHeight+(d.maxHeight-d.minHeight)*t;auto color=(d.color&0xffffff)|(std::uint32_t((d.color>>24)*(1-t))<<24);V3 pos{x,water?w+d.offset:g+height*.5f,z};quad(projection,pos,water?V3{width*.5f,0,0}:mul(side,width*.5f),water?V3{0,0,height*.5f}:V3{0,height*.5f,0},color,texture);}
  }
 }
 unsigned animatedTexture(const std::string &name,float seconds){
#ifdef REIGN_BINK
  auto stem=name.substr(0,name.find_last_of('.'));if(stem=="movie-billboard"){
   auto &m=movies[stem];if(!m.decoder)m.decoder=std::make_unique<BinkMovie>(loader(stem+".bik"));float t=std::fmod(seconds,float(m.decoder->duration()));if(t<m.last){m.decoder=std::make_unique<BinkMovie>(loader(stem+".bik"));m.frame=-1;}m.last=t;m.decoder->advance(t);m.decoder->drain_audio();if(m.frame!=m.decoder->frame_index()){if(m.texture)renderer.update_texture_rgba(m.texture,m.decoder->width(),m.decoder->height(),m.decoder->rgba().data());else m.texture=renderer.texture_rgba(m.decoder->width(),m.decoder->height(),m.decoder->rgba().data());m.frame=m.decoder->frame_index();}return m.texture;
  }
#endif
 auto first=bitmaps->load(name);auto frames=bitmaps->animation(name);return frames.empty()?first->texture:frames[std::size_t(std::floor(seconds*std::max(0.f,terrain.environment.animationRate)))%frames.size()]->texture;}
 void drawSky(const WorldProjection &projection,V3 camera,const ShellLightSample &light,float seconds){
  auto sun=projection.transform(add(camera,mul(light.celestialDirection,farPlane)));if(sun[3]>0){float x=(sun[0]/sun[3]+1)*W*.5f,y=(1+sun[1]/sun[3])*H*.5f,r=W*.05f;auto color=packed(light.sun[0],light.sun[1],light.sun[2]);LegacyTL v[]={{x-r,y-r,1,1,color,0xff000000,0,0},{x+r,y-r,1,1,color,0xff000000,1,0},{x+r,y+r,1,1,color,0xff000000,1,1},{x-r,y+r,1,1,color,0xff000000,0,1}};std::uint16_t ix[]={0,1,2,0,2,3};DrawState state;state.blend=Blend::alpha;state.filtered=true;state.texture_op=TextureOp::modulate_rgba;state.texture=animatedTexture(light.sunUp?"engine_sun.pic":"engine_moon.pic",seconds);renderer.draw(W,H,v,4,ix,6,state);}

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
std::vector<std::string> ShellScene::take_sounds(){std::vector<std::string> result;result.swap(impl_->sounds);return result;}
ShellEffectStats ShellScene::effect_stats()const{return impl_->stats;}
std::string ShellScene::ambient_sound()const{return impl_->rainActive?impl_->rainSound:std::string{};}
void ShellScene::finish_frame(float seconds){
 // Original StepHoldPrim Time(0,1,4), Direction("up"), black fade after IFace.
 float alpha=seconds<1?1:seconds<4?(4-seconds)/3:0;if(alpha<=0)return;
 LegacyTL v[]={{0,0,0,1,packed(0,0,0,alpha),0xff000000,0,0},{640,0,0,1,packed(0,0,0,alpha),0xff000000,0,0},{640,480,0,1,packed(0,0,0,alpha),0xff000000,0,0},{0,480,0,1,packed(0,0,0,alpha),0xff000000,0,0}};std::uint16_t indices[]={0,1,2,0,2,3};DrawState state;state.blend=Blend::alpha;impl_->renderer.draw(640,480,v,4,indices,6,state);
}
}

