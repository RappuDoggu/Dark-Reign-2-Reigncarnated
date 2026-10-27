#include "reign/legacy_gles.hpp"
#include <GLES3/gl3.h>
#include <cmath>
#include <stdexcept>
#include <string>
#include <limits>
#include <algorithm>
#include <vector>
namespace reign {
#define REIGN_GL(X) \
 X(CreateShader,PFNGLCREATESHADERPROC) X(ShaderSource,PFNGLSHADERSOURCEPROC) X(CompileShader,PFNGLCOMPILESHADERPROC) \
 X(GetShaderiv,PFNGLGETSHADERIVPROC) X(GetShaderInfoLog,PFNGLGETSHADERINFOLOGPROC) X(DeleteShader,PFNGLDELETESHADERPROC) \
 X(CreateProgram,PFNGLCREATEPROGRAMPROC) X(AttachShader,PFNGLATTACHSHADERPROC) X(LinkProgram,PFNGLLINKPROGRAMPROC) \
 X(GetProgramiv,PFNGLGETPROGRAMIVPROC) X(GetProgramInfoLog,PFNGLGETPROGRAMINFOLOGPROC) X(DeleteProgram,PFNGLDELETEPROGRAMPROC) \
 X(UseProgram,PFNGLUSEPROGRAMPROC) X(GetUniformLocation,PFNGLGETUNIFORMLOCATIONPROC) X(Uniform1i,PFNGLUNIFORM1IPROC) \
 X(Uniform1f,PFNGLUNIFORM1FPROC) X(Uniform2f,PFNGLUNIFORM2FPROC) X(Uniform3fv,PFNGLUNIFORM3FVPROC) \
 X(GenVertexArrays,PFNGLGENVERTEXARRAYSPROC) X(BindVertexArray,PFNGLBINDVERTEXARRAYPROC) X(DeleteVertexArrays,PFNGLDELETEVERTEXARRAYSPROC) \
 X(GenBuffers,PFNGLGENBUFFERSPROC) X(BindBuffer,PFNGLBINDBUFFERPROC) X(BufferData,PFNGLBUFFERDATAPROC) X(DeleteBuffers,PFNGLDELETEBUFFERSPROC) \
 X(EnableVertexAttribArray,PFNGLENABLEVERTEXATTRIBARRAYPROC) X(VertexAttribPointer,PFNGLVERTEXATTRIBPOINTERPROC) \
 X(GenTextures,PFNGLGENTEXTURESPROC) X(BindTexture,PFNGLBINDTEXTUREPROC) X(TexParameteri,PFNGLTEXPARAMETERIPROC) \
 X(TexImage2D,PFNGLTEXIMAGE2DPROC) X(DeleteTextures,PFNGLDELETETEXTURESPROC) X(ActiveTexture,PFNGLACTIVETEXTUREPROC) \
 X(Viewport,PFNGLVIEWPORTPROC) X(Enable,PFNGLENABLEPROC) X(Disable,PFNGLDISABLEPROC) X(DepthMask,PFNGLDEPTHMASKPROC) \
 X(DepthFunc,PFNGLDEPTHFUNCPROC) X(BlendFunc,PFNGLBLENDFUNCPROC) X(ClearColor,PFNGLCLEARCOLORPROC) \
 X(ClearDepthf,PFNGLCLEARDEPTHFPROC) X(Clear,PFNGLCLEARPROC) X(DrawElements,PFNGLDRAWELEMENTSPROC) \
 X(GenFramebuffers,PFNGLGENFRAMEBUFFERSPROC) X(DeleteFramebuffers,PFNGLDELETEFRAMEBUFFERSPROC) X(BindFramebuffer,PFNGLBINDFRAMEBUFFERPROC) \
 X(FramebufferTexture2D,PFNGLFRAMEBUFFERTEXTURE2DPROC) X(CheckFramebufferStatus,PFNGLCHECKFRAMEBUFFERSTATUSPROC) \
 X(GenRenderbuffers,PFNGLGENRENDERBUFFERSPROC) X(DeleteRenderbuffers,PFNGLDELETERENDERBUFFERSPROC) X(BindRenderbuffer,PFNGLBINDRENDERBUFFERPROC) \
 X(RenderbufferStorage,PFNGLRENDERBUFFERSTORAGEPROC) X(FramebufferRenderbuffer,PFNGLFRAMEBUFFERRENDERBUFFERPROC) X(BlitFramebuffer,PFNGLBLITFRAMEBUFFERPROC) \
 X(ReadPixels,PFNGLREADPIXELSPROC) X(GetError,PFNGLGETERRORPROC) X(PixelStorei,PFNGLPIXELSTOREIPROC)
struct LegacyGles::Impl {
#define DECLARE(name,type) type name=nullptr;
 REIGN_GL(DECLARE)
#undef DECLARE
 GLuint program=0,vao=0,vbo=0,ibo=0,white=0,fbo=0,color=0,depth=0;
 unsigned width=0,height=0;
 explicit Impl(Resolver resolver) {
#define LOAD(name,type) name=reinterpret_cast<type>(resolver("gl" #name));if(!name)throw std::runtime_error("Missing GLES entry: gl" #name);
  REIGN_GL(LOAD)
#undef LOAD
 }
 void check(){if(auto error=GetError())throw std::runtime_error("GLES error "+std::to_string(error));}
 void target(unsigned w,unsigned h){
  if(!w||!h||w>16384||h>16384||std::uint64_t(w)*h*4>128*1024*1024)throw std::runtime_error("Invalid native render target size");
  if(w==width&&h==height&&fbo){BindFramebuffer(GL_FRAMEBUFFER,fbo);return;}
  GLuint next_fbo=0,next_color=0,next_depth=0;
  try{
   GenFramebuffers(1,&next_fbo);GenTextures(1,&next_color);GenRenderbuffers(1,&next_depth);
   BindTexture(GL_TEXTURE_2D,next_color);TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
   TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
   TexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
   BindRenderbuffer(GL_RENDERBUFFER,next_depth);RenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT16,w,h);
   BindFramebuffer(GL_FRAMEBUFFER,next_fbo);FramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,next_color,0);
   FramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,next_depth);check();
   if(CheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("Native render target incomplete");
  }catch(...){if(next_fbo)DeleteFramebuffers(1,&next_fbo);if(next_color)DeleteTextures(1,&next_color);if(next_depth)DeleteRenderbuffers(1,&next_depth);throw;}
  if(fbo)DeleteFramebuffers(1,&fbo);if(color)DeleteTextures(1,&color);if(depth)DeleteRenderbuffers(1,&depth);
  fbo=next_fbo;color=next_color;depth=next_depth;width=w;height=h;
 }
 void read_size(unsigned w,unsigned h,const void *out){if(!out||w!=width||h!=height||!fbo)throw std::runtime_error("Readback must match the native render target");}
 GLuint shader(GLenum type,const char *source){
  auto s=CreateShader(type);ShaderSource(s,1,&source,nullptr);CompileShader(s);GLint ok=0;GetShaderiv(s,GL_COMPILE_STATUS,&ok);
  if(!ok){char log[4096];GetShaderInfoLog(s,sizeof(log),nullptr,log);DeleteShader(s);throw std::runtime_error(log);}return s;
 }
 GLint uniform(const char *name){return GetUniformLocation(program,name);}
 ~Impl(){if(fbo)DeleteFramebuffers(1,&fbo);if(color)DeleteTextures(1,&color);if(depth)DeleteRenderbuffers(1,&depth);if(white)DeleteTextures(1,&white);if(ibo)DeleteBuffers(1,&ibo);if(vbo)DeleteBuffers(1,&vbo);if(vao)DeleteVertexArrays(1,&vao);if(program)DeleteProgram(program);}
};
LegacyGles::LegacyGles(Resolver resolver):impl_(std::make_unique<Impl>(resolver)){
 auto &g=*impl_;
 const char *vertex=R"(#version 300 es
precision highp float;
layout(location=0) in vec4 position;
layout(location=1) in vec4 diffuse_bgra;
layout(location=2) in vec4 specular_bgra;
layout(location=3) in vec2 uv;
uniform vec2 viewport_size;
uniform int homogeneous_input;
out vec2 texcoord;
out vec4 screen_diffuse;
out vec4 screen_specular;
out float screen_w;
void main(){
 float w;
 if(homogeneous_input!=0){
  w=position.w;
  gl_Position=vec4(position.xy+vec2(w)/viewport_size,2.0*position.z-w,w);
 }else{
  w=1.0/position.w;
  vec2 ndc=vec2((position.x+0.5)*2.0/viewport_size.x-1.0,(position.y+0.5)*2.0/viewport_size.y-1.0);
  gl_Position=vec4(ndc*w,(position.z*2.0-1.0)*w,w);
 }
 texcoord=uv;
 screen_diffuse=diffuse_bgra.bgra*w;
 screen_specular=specular_bgra.bgra*w;
 screen_w=w;
})";
 const char *fragment=R"(#version 300 es
precision highp float;
in vec2 texcoord;
in vec4 screen_diffuse;
in vec4 screen_specular;
in float screen_w;
uniform sampler2D image;
uniform int texture_op;
uniform int fog_enabled;
uniform int specular_enabled;
uniform int alpha_test;
uniform float alpha_reference;
uniform vec3 fog_rgb;
out vec4 output_color;
void main(){
 vec4 d=screen_diffuse/screen_w;
 vec4 s=screen_specular/screen_w;
 vec4 t=texture(image,texcoord);
 vec4 c=d;
 if(texture_op==1)c=d*t;
 if(texture_op==2)c=vec4(d.rgb*t.rgb,t.a);
 if(texture_op==3)c=vec4(t.rgb,t.a*d.a);
 if(texture_op==4)c=vec4(d.rgb,t.a);
 if(texture_op==5)c=t;
 if(texture_op==6)c=vec4(clamp(d.rgb*t.rgb*2.0,0.0,1.0),d.a*t.a);
 if(texture_op==7)c=vec4(clamp(d.rgb*t.rgb*4.0,0.0,1.0),d.a*t.a);
 if(alpha_test!=0 && c.a<=alpha_reference)discard;
 if(specular_enabled!=0)c.rgb=clamp(c.rgb+s.rgb,0.0,1.0);
 if(fog_enabled!=0)c.rgb=mix(fog_rgb,c.rgb,s.a);
 output_color=c;
})";
 auto vs=g.shader(GL_VERTEX_SHADER,vertex);GLuint fs=0;
 try { fs=g.shader(GL_FRAGMENT_SHADER,fragment); } catch(...) {g.DeleteShader(vs);throw;}
 g.program=g.CreateProgram();g.AttachShader(g.program,vs);g.AttachShader(g.program,fs);g.LinkProgram(g.program);g.DeleteShader(vs);g.DeleteShader(fs);
 GLint ok=0;g.GetProgramiv(g.program,GL_LINK_STATUS,&ok);if(!ok){char log[4096];g.GetProgramInfoLog(g.program,sizeof(log),nullptr,log);throw std::runtime_error(log);}
 g.GenVertexArrays(1,&g.vao);g.GenBuffers(1,&g.vbo);g.GenBuffers(1,&g.ibo);
 const std::uint8_t white[]={255,255,255,255};g.white=texture_rgba(1,1,white);g.check();
}
LegacyGles::~LegacyGles()=default;
unsigned LegacyGles::texture_rgba(unsigned width,unsigned height,const std::uint8_t *data){
 if(!width||!height||width>16384||height>16384||!data)throw std::runtime_error("Invalid RGBA texture");
 auto &g=*impl_;GLuint id=0;g.GenTextures(1,&id);g.BindTexture(GL_TEXTURE_2D,id);g.PixelStorei(GL_UNPACK_ALIGNMENT,1);
 g.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);g.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 g.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);g.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
 g.TexImage2D(GL_TEXTURE_2D,0,GL_RGBA, width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,data);g.check();return id;
}
void LegacyGles::abandon_context() noexcept { auto &g=*impl_;g.program=g.vao=g.vbo=g.ibo=g.white=g.fbo=g.color=g.depth=0;g.width=g.height=0; }
void LegacyGles::delete_texture(unsigned id){if(id)impl_->DeleteTextures(1,&id);}
void LegacyGles::clear(unsigned w,unsigned h,float r,float g,float b,float a){
 auto &gl=*impl_;gl.target(w,h);gl.Viewport(0,0,w,h);gl.Disable(GL_SCISSOR_TEST);gl.DepthMask(GL_TRUE);gl.ClearColor(r,g,b,a);gl.ClearDepthf(1);gl.Clear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);gl.check();
}
void LegacyGles::draw(unsigned w,unsigned h,const LegacyTL *vertices,std::size_t count,const std::uint16_t *indices,std::size_t index_count,const DrawState &state){
 if(!vertices||count>65536)throw std::runtime_error("Invalid TL batch");
 for(std::size_t i=0;i<count;++i)if(!std::isfinite(vertices[i].rhw)||vertices[i].rhw<=0||!std::isfinite(vertices[i].x)||!std::isfinite(vertices[i].y)||!std::isfinite(vertices[i].z)||!std::isfinite(vertices[i].u)||!std::isfinite(vertices[i].v))throw std::runtime_error("Invalid TL coordinates/rhw");
 draw_batch(w,h,vertices,count,indices,index_count,state,false);
}
void LegacyGles::draw_clip(unsigned w,unsigned h,const LegacyClip *vertices,std::size_t count,const std::uint16_t *indices,std::size_t index_count,const DrawState &state){
 if(!vertices||count>65536)throw std::runtime_error("Invalid homogeneous batch");
 for(std::size_t i=0;i<count;++i){const auto &v=vertices[i];if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z)||!std::isfinite(v.w)||!std::isfinite(v.u)||!std::isfinite(v.v))throw std::runtime_error("Invalid homogeneous coordinates");}
 draw_batch(w,h,vertices,count,indices,index_count,state,true);
}
void LegacyGles::draw_batch(unsigned w,unsigned h,const void *vertices,std::size_t count,const std::uint16_t *indices,std::size_t index_count,const DrawState &state,bool homogeneous){
 if(!w||!h||!vertices||!indices||!count||count>65536||!index_count||index_count%3||index_count>std::size_t(std::numeric_limits<GLsizei>::max()))throw std::runtime_error("Unsupported TL triangle batch");
 if(static_cast<unsigned>(state.texture_op)>7 || static_cast<unsigned>(state.blend)>3 || static_cast<unsigned>(state.address)>2 || static_cast<unsigned>(state.source_factor)<1 || static_cast<unsigned>(state.source_factor)>11 || static_cast<unsigned>(state.destination_factor)<1 || static_cast<unsigned>(state.destination_factor)>10 || !std::isfinite(state.alpha_reference) || state.alpha_reference<0 || state.alpha_reference>1)throw std::runtime_error("Unsupported legacy render state");
 for(std::size_t i=0;i<index_count;++i)if(indices[i]>=count)throw std::runtime_error("TL index out of bounds");
 auto &g=*impl_;g.target(w,h);g.Viewport(0,0,w,h);g.Disable(GL_CULL_FACE);g.Disable(GL_SCISSOR_TEST);
 if(state.depth_test)g.Enable(GL_DEPTH_TEST);else g.Disable(GL_DEPTH_TEST);g.DepthMask(state.depth_write?GL_TRUE:GL_FALSE);g.DepthFunc(GL_LEQUAL);
 if(state.blend==Blend::opaque)g.Disable(GL_BLEND);else{g.Enable(GL_BLEND);g.BlendFunc(GL_SRC_ALPHA,state.blend==Blend::alpha?GL_ONE_MINUS_SRC_ALPHA:GL_ONE);}
 if(state.blend==Blend::custom){
  const GLenum factors[]={GL_ZERO,GL_ONE,GL_SRC_COLOR,GL_ONE_MINUS_SRC_COLOR,GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_DST_ALPHA,GL_ONE_MINUS_DST_ALPHA,GL_DST_COLOR,GL_ONE_MINUS_DST_COLOR,GL_SRC_ALPHA_SATURATE};
  g.BlendFunc(factors[static_cast<unsigned>(state.source_factor)-1],factors[static_cast<unsigned>(state.destination_factor)-1]);
 }
 g.UseProgram(g.program);g.Uniform1i(g.uniform("homogeneous_input"),homogeneous);g.Uniform2f(g.uniform("viewport_size"),w,h);g.Uniform1i(g.uniform("texture_op"),static_cast<int>(state.texture_op));
 g.Uniform1i(g.uniform("fog_enabled"),state.fog);g.Uniform1i(g.uniform("specular_enabled"),state.specular);g.Uniform3fv(g.uniform("fog_rgb"),1,state.fog_rgb);g.Uniform1i(g.uniform("alpha_test"),state.alpha_test);g.Uniform1f(g.uniform("alpha_reference"),state.alpha_reference);
 g.ActiveTexture(GL_TEXTURE0);g.BindTexture(GL_TEXTURE_2D,state.texture?state.texture:g.white);g.Uniform1i(g.uniform("image"),0);
 g.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,state.filtered?GL_LINEAR:GL_NEAREST);g.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,state.filtered?GL_LINEAR:GL_NEAREST);
 const GLint addressing=state.address==TextureAddress::wrap?GL_REPEAT:state.address==TextureAddress::mirror?GL_MIRRORED_REPEAT:GL_CLAMP_TO_EDGE;
 g.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,addressing);g.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,addressing);
 g.BindVertexArray(g.vao);g.BindBuffer(GL_ARRAY_BUFFER,g.vbo);g.BufferData(GL_ARRAY_BUFFER,count*sizeof(LegacyTL),vertices,GL_STREAM_DRAW);
 g.BindBuffer(GL_ELEMENT_ARRAY_BUFFER,g.ibo);g.BufferData(GL_ELEMENT_ARRAY_BUFFER,index_count*sizeof(std::uint16_t),indices,GL_STREAM_DRAW);
 for(unsigned i=0;i<4;++i)g.EnableVertexAttribArray(i);
 g.VertexAttribPointer(0,4,GL_FLOAT,GL_FALSE,sizeof(LegacyTL),nullptr);
 g.VertexAttribPointer(1,4,GL_UNSIGNED_BYTE,GL_TRUE,sizeof(LegacyTL),reinterpret_cast<void *>(16));
 g.VertexAttribPointer(2,4,GL_UNSIGNED_BYTE,GL_TRUE,sizeof(LegacyTL),reinterpret_cast<void *>(20));
 g.VertexAttribPointer(3,2,GL_FLOAT,GL_FALSE,sizeof(LegacyTL),reinterpret_cast<void *>(24));
 g.DrawElements(GL_TRIANGLES,index_count,GL_UNSIGNED_SHORT,nullptr);g.check();
}
void LegacyGles::present(){
 auto &g=*impl_;if(!g.fbo)throw std::runtime_error("No native frame to present");
 g.Disable(GL_SCISSOR_TEST);g.BindFramebuffer(GL_READ_FRAMEBUFFER,g.fbo);g.BindFramebuffer(GL_DRAW_FRAMEBUFFER,0);
 g.BlitFramebuffer(0,0,g.width,g.height,0,g.height,g.width,0,GL_COLOR_BUFFER_BIT,GL_NEAREST);g.BindFramebuffer(GL_FRAMEBUFFER,0);g.check();
}
void LegacyGles::present_to(unsigned width,unsigned height){
 auto &g=*impl_;if(!g.fbo||!width||!height||width>32768||height>32768)throw std::invalid_argument("Invalid presentation surface");
 double scale=std::min(double(width)/g.width,double(height)/g.height);int w=int(g.width*scale),h=int(g.height*scale),x=(int(width)-w)/2,y=(int(height)-h)/2;
 g.Disable(GL_SCISSOR_TEST);g.BindFramebuffer(GL_FRAMEBUFFER,0);g.ClearColor(0,0,0,1);g.Clear(GL_COLOR_BUFFER_BIT);
 g.BindFramebuffer(GL_READ_FRAMEBUFFER,g.fbo);g.BindFramebuffer(GL_DRAW_FRAMEBUFFER,0);
 g.BlitFramebuffer(0,0,g.width,g.height,x,y+h,x+w,y,GL_COLOR_BUFFER_BIT,GL_NEAREST);g.BindFramebuffer(GL_FRAMEBUFFER,0);g.check();
}
void LegacyGles::read_presented_rgba(unsigned w,unsigned h,std::uint8_t *out){auto &g=*impl_;g.read_size(w,h,out);g.BindFramebuffer(GL_FRAMEBUFFER,0);g.PixelStorei(GL_PACK_ALIGNMENT,1);g.ReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,out);g.check();}
void LegacyGles::read_rgba(unsigned w,unsigned h,std::uint8_t *out){
 auto &g=*impl_;g.read_size(w,h,out);g.BindFramebuffer(GL_FRAMEBUFFER,g.fbo);g.PixelStorei(GL_PACK_ALIGNMENT,1);g.ReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,out);g.check();
 const std::size_t pitch=std::size_t(w)*4;for(unsigned y=0;y<h/2;++y)std::swap_ranges(out+y*pitch,out+(y+1)*pitch,out+(h-1-y)*pitch);
}
}
