package org.reigncarnated.menu;

import java.io.*;
import java.util.*;
import java.util.function.BooleanSupplier;
import java.util.function.Consumer;

/** Copies only this milestone's required data into an isolated, validated snapshot.
 * Caller publishes the returned directory in preferences only after success. */
final class DataFolderImporter {
 static final long MAX_FILE=512L*1024*1024, MAX_TOTAL=2L*1024*1024*1024;
 static final class Node {
  final String id,name; final boolean directory; final long size;
  Node(String id,String name,boolean directory,long size){this.id=id;this.name=name;this.directory=directory;this.size=size;}
 }
 interface Source {List<Node> children(Node directory)throws IOException;InputStream open(Node file)throws IOException;}
 static final class Result {
  final File directory; final int tracks,scenePacks;
  Result(File directory,int tracks,int scenePacks){this.directory=directory;this.tracks=tracks;this.scenePacks=scenePacks;}
 }
 private final Source source; private final BooleanSupplier cancelled; private final Consumer<String> progress;
 private long total,lastProgress;
 DataFolderImporter(Source source,BooleanSupplier cancelled,Consumer<String> progress){this.source=source;this.cancelled=cancelled;this.progress=progress;}
 private static final class ScenePack {
  final Node node;final String destination,required;
  ScenePack(Node node,String destination,String required){this.node=node;this.destination=destination;this.required=required;}
 }
 private void optionalPack(List<ScenePack> out,Node parent,String name,String destination,String required)throws IOException{
  if(parent!=null){Node file=child(parent,name,false,false);if(file!=null)out.add(new ScenePack(file,destination,required));}
 }
 private void check()throws IOException{if(cancelled.getAsBoolean()||Thread.currentThread().isInterrupted())throw new IOException("Import cancelled");}
 private Node child(Node directory,String name,boolean isDirectory,boolean required)throws IOException{
  check();Node found=null;
  for(Node n:source.children(directory))if(n.name.equalsIgnoreCase(name)){
   if(found!=null)throw new IOException("Ambiguous duplicate: "+name);
   if(n.directory!=isDirectory)throw new IOException(name+" must be a "+(isDirectory?"folder":"file"));found=n;
  }
  if(found==null&&required)throw new IOException("Missing "+name+". Select the Dark Reign 2 installation folder containing packs, missions and music.");
  return found;
 }
 Result run(Node selected,File parent)throws IOException{
  check();if(!selected.directory)throw new IOException("Select a folder, not an archive");
  Node packs=child(selected,"packs",true,true),missions=child(selected,"missions",true,true);
  Node system=child(missions,"system",true,true);
  Node base=child(packs,"base.zwp",false,true),shell=child(system,"shell.zwp",false,true);
  List<ScenePack> scene=new ArrayList<>();
  Node worlds=child(selected,"worlds",true,false),sides=child(selected,"sides",true,false);
  Node client=sides==null?null:child(sides,"client",true,false);
  optionalPack(scene,worlds,"sprawl.zwp","worlds/sprawl.zwp","types_world.cfg");
  optionalPack(scene,client,"jda.zwp","sides/client/jda.zwp","interface_game_side.cfg");
  optionalPack(scene,client,"sprawler.zwp","sides/client/sprawler.zwp","interface_game_side.cfg");
  Node music=child(selected,"music",true,false);List<Node> tracks=new ArrayList<>();
  if(music!=null)for(Node n:source.children(music)){
   String lower=n.name.toLowerCase(Locale.ROOT);
   if(!n.directory&&(lower.endsWith(".ogg")||lower.endsWith(".mp3")||lower.endsWith(".wav")))tracks.add(n);
  }
  if(tracks.size()>128)throw new IOException("Music folder has more than 128 tracks");
  tracks.sort((a,b)->a.name.compareToIgnoreCase(b.name));
  long known=0;List<Node> all=new ArrayList<>();all.add(base);all.add(shell);for(ScenePack pack:scene)all.add(pack.node);all.addAll(tracks);
  for(Node n:all){if(n.size>MAX_FILE)throw new IOException("File exceeds 512 MiB: "+n.name);if(n.size>0)known+=n.size;}
  if(known>MAX_TOTAL)throw new IOException("Menu data exceeds 2 GiB");
  if(!parent.isDirectory()&&!parent.mkdirs())throw new IOException("Cannot create app data directory");
  long free=parent.getUsableSpace();if(free>0&&known+32L*1024*1024>free)throw new IOException("Not enough free storage for a safe import; the previous installation will be kept");
  String token=UUID.randomUUID().toString();File staging=new File(parent,"import-"+token),ready=new File(parent,"data-"+token);
  if(!staging.mkdir())throw new IOException("Cannot create import directory");boolean complete=false;
  try{
   copy(base,new File(staging,"base.zwp"));validatePack(new File(staging,"base.zwp"),"runcode_main_login.cfg","english.cfg");
   copy(shell,new File(staging,"shell.zwp"));validatePack(new File(staging,"shell.zwp"),"interface_shell.cfg");
   for(ScenePack pack:scene){File destination=new File(staging,pack.destination);if(!destination.getParentFile().isDirectory()&&!destination.getParentFile().mkdirs())throw new IOException("Cannot create scene directory");copy(pack.node,destination);validatePack(destination,pack.required);}
   File musicOut=new File(staging,"music");if(!musicOut.mkdir())throw new IOException("Cannot create music directory");
   int i=0;for(Node track:tracks){String ext=track.name.substring(track.name.lastIndexOf('.')).toLowerCase(Locale.ROOT);copy(track,new File(musicOut,String.format(Locale.ROOT,"%03d%s",i++,ext)));}
   copyRuntimeTree(selected,staging,"",0);
   check();if(!staging.renameTo(ready))throw new IOException("Cannot finalize imported data");
   complete=true;return new Result(ready,tracks.size(),scene.size());
  }finally{if(!complete)remove(staging);}
 }
 private void copyRuntimeTree(Node dir,File target,String relative,int depth)throws IOException{
  if(depth>16)throw new IOException("Game data folders nested too deeply");
  Set<String> names=new HashSet<>();
  for(Node node:source.children(dir)){
   check();String name=node.name.toLowerCase(Locale.ROOT);
   if(name.isEmpty()||name.equals(".")||name.equals("..")||name.indexOf('/')>=0||name.indexOf('\\')>=0||name.indexOf(0)>=0||name.indexOf(':')>=0||!names.add(name))throw new IOException("Invalid/duplicate runtime path");
   String path=relative.isEmpty()?name:relative+"/"+name;
   if(path.equals("packs/base.zwp")||path.equals("missions/system/shell.zwp")||path.equals("worlds/sprawl.zwp")||path.equals("sides/client/jda.zwp")||path.equals("sides/client/sprawler.zwp")||path.equals("music"))continue;
   if(relative.isEmpty()&&node.directory&&!Arrays.asList("packs","missions","worlds","sides","mods","movies","data").contains(name))continue;
   if(node.directory){File next=new File(target,name);if(!next.isDirectory()&&!next.mkdirs())throw new IOException("Cannot create runtime directory");copyRuntimeTree(node,next,path,depth+1);}
   else if(name.matches(".*\\.(zwp|cfg|bik|ogg|mp3|wav|tga|bmp|pic|god|blk|fff|xsi|txt)")){File out=new File(target,name);copy(node,out);if(name.endsWith(".zwp"))validatePack(out);}
  }
 }
 private void copy(Node node,File destination)throws IOException{
  check();progress.accept("Importing "+node.name+"…");long count=0;
  try(InputStream input=source.open(node);FileOutputStream out=new FileOutputStream(destination)){
   if(input==null)throw new IOException("Cannot read "+node.name);byte[] buffer=new byte[262144];int n;
   while((n=input.read(buffer))!=-1){check();count+=n;total+=n;if(count>MAX_FILE||total>MAX_TOTAL)throw new IOException("Imported data exceeds storage limit");out.write(buffer,0,n);
    if(total-lastProgress>=8L*1024*1024){lastProgress=total;progress.accept("Importing game data: "+(total/1024/1024)+" MiB copied");}
   }
   out.getFD().sync();
  }
  if(count==0||(node.size>=0&&count!=node.size))throw new IOException("Incomplete file: "+node.name);
 }
 private static long u32(RandomAccessFile f)throws IOException{return Integer.toUnsignedLong(Integer.reverseBytes(f.readInt()));}
 static void validatePack(File file,String... required)throws IOException{
  try(RandomAccessFile f=new RandomAccessFile(file,"r")){
   long bytes=f.length();if(bytes<56||f.readInt()!=0x4e4f524b||u32(f)!=1)throw new IOException("Invalid Dark Reign 2 archive: "+file.getName());
   long dirs=u32(f),directoryOffset=u32(f),files=u32(f),fileOffset=u32(f);
   if(fileOffset<56||directoryOffset<fileOffset||directoryOffset>bytes||files>(directoryOffset-fileOffset)/14||files>1000000||dirs>(bytes-directoryOffset)/2||directoryOffset-fileOffset>64L*1024*1024)throw new IOException("Invalid archive tables: "+file.getName());
   Set<String> names=new HashSet<>();f.seek(fileOffset);
   for(long i=0;i<files;i++){
    if(f.getFilePointer()+5>directoryOffset)throw new IOException("Truncated archive table");
    long dir=u32(f);int length=f.readUnsignedByte();if(length==0||f.getFilePointer()+length+8>directoryOffset)throw new IOException("Invalid archive name");
    byte[] raw=new byte[length];f.readFully(raw);String name=new String(raw,"ISO-8859-1").toLowerCase(Locale.ROOT);
    long offset=u32(f),size=u32(f);
    if(dir>dirs||offset<56||offset+size>fileOffset||name.indexOf('/')>=0||name.indexOf('\\')>=0||name.indexOf(0)>=0||!names.add(name))throw new IOException("Invalid archive entry");
   }
   f.seek(directoryOffset);for(long i=0;i<dirs;i++){int length=f.readUnsignedByte();if(length==0||f.getFilePointer()+length>bytes)throw new IOException("Invalid archive directory");for(int j=0;j<length;j++)if(f.readByte()==0)throw new IOException("Invalid archive directory name");}
   for(String name:required)if(!names.contains(name))throw new IOException(file.getName()+" is missing "+name+"; select a complete installation");
  }
 }
 static void remove(File file){if(file.isDirectory()){File[] children=file.listFiles();if(children!=null)for(File child:children)remove(child);}file.delete();}
 static List<String> playlist(File directory){List<String> tracks=new ArrayList<>();File[] files=new File(directory,"music").listFiles();if(files!=null){Arrays.sort(files,Comparator.comparing(File::getName));for(File file:files)if(file.isFile())tracks.add(file.getAbsolutePath());}return tracks;}
}
