#pragma once
#include "reign/config_prelude.hpp"
#include "reign/bitmap_manager.hpp"
#include "ntree.h"
namespace reign::bitmap_manager_detail {
using BuffString=StrBuf<256>;using FileDrive=StrBuf<256>;using FileDir=StrBuf<256>;using FileName=StrBuf<256>;using FileExt=StrBuf<256>;
namespace Utils {char *FindExt(const char *);}
namespace Dir {void PathExpand(const char *,FileDrive &,FileDir &,FileName &,FileExt &);}
namespace FileSys {Bool Exists(const char *);}
constexpr U32 bitmapTEXTURE=2;
class Bitmap {
public:
 struct Manager {
  static void Setup(U32,Bitmap &,const char *,U32,U32,U32,Bool);
  static Bitmap *Create(U32,const char *,U32,U32,U32,Bool);
  static Bitmap *Find(const char *);
  static Bitmap *FindCreate(U32,const char *,U32,U32,U32,Bool,Bool);
  static void ReleaseDD();static void OnModeChange();static void ClearTextures();static void ResetData();
 };
 NBinTree<Bitmap>::Node treeNode;
 struct {Bool transparent=FALSE;} status;
 U32 type=2,mipMapCount=0,stage=0,reduction=0,animFrame=0;
 Bitmap *next=nullptr;std::string name;
 std::shared_ptr<ManagedBitmap> record=std::make_shared<ManagedBitmap>();
 Bitmap();~Bitmap();void SetName(const char *s){name=s;}
 void Read(const char *);void ReLoad();void ReleaseDD();
 void SetNext(Bitmap *p){next=p;}
};
struct State {
 NBinTree<Bitmap> tree{&Bitmap::treeNode};ConfigLoader loader;BitmapManager::Exists exists;
 BitmapGpuCallbacks gpu;std::vector<Bitmap *> pending;
};
State &state();
}
