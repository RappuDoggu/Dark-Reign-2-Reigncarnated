package org.reigncarnated.menu;

import java.io.*;
import java.nio.file.*;
import java.security.*;
import java.util.*;
import java.util.function.*;
import java.util.zip.*;

/** Exports only an imported runtime snapshot and its selected music, never app profiles/keys. */
final class GameDataZip {
 static final class Result {
  final File file;final int files,tracks;final long bytes;final String sha256;
  Result(File file,int files,int tracks,long bytes,String sha256){this.file=file;this.files=files;this.tracks=tracks;this.bytes=bytes;this.sha256=sha256;}
 }
 private static final String[] TREES={"packs","missions","worlds","sides","mods","movies","data","library"};
 private static List<String> tracks(File root){List<String> out=new ArrayList<>();if(root!=null)for(String path:DataFolderImporter.playlist(root)){File file=new File(path);if(file.length()>0&&file.getName().toLowerCase(Locale.ROOT).matches(".*\\.(ogg|mp3|wav)"))out.add(path);}return out;}
 static File musicRoot(File installation,File preferred){
  if(!tracks(preferred).isEmpty())return preferred;
  return !tracks(installation).isEmpty()?installation:null;
 }
 private static void check(BooleanSupplier cancelled)throws IOException{if(cancelled.getAsBoolean()||Thread.currentThread().isInterrupted())throw new IOException("Export cancelled");}
 private static void add(Map<String,File> entries,String name,File file)throws IOException{
  if(Files.isSymbolicLink(file.toPath())||!file.isFile())throw new IOException("Unsupported runtime file: "+name);
  if(file.length()==0||file.length()>DataFolderImporter.MAX_FILE)throw new IOException("Empty or oversized runtime file: "+name);
  if(entries.putIfAbsent(name,file)!=null)throw new IOException("Duplicate runtime path: "+name);
  if(entries.size()>100000)throw new IOException("Too many runtime files");
 }
 private static void walk(Map<String,File> entries,File dir,String prefix,int depth)throws IOException{
  if(depth>16||Files.isSymbolicLink(dir.toPath()))throw new IOException("Unsupported runtime directory: "+prefix);
  File[] files=dir.listFiles();if(files==null)throw new IOException("Cannot read runtime directory: "+prefix);
  for(File file:files){String n=file.getName().toLowerCase(Locale.ROOT);
   if(n.isEmpty()||n.equals(".")||n.equals("..")||n.matches(".*[\\\\/:\\x00\\t\\r\\n].*"))throw new IOException("Invalid runtime filename");
   String path=prefix+"/"+n;if(Files.isSymbolicLink(file.toPath()))throw new IOException("Runtime links cannot be exported");
   if(file.isDirectory())walk(entries,file,path,depth+1);
   else if(n.matches(".*\\.(zwp|cfg|bik|ogg|mp3|wav|tga|bmp|pic|god|blk|fff|xsi|txt)"))add(entries,path,file);
  }
 }
 private static File required(Map<String,File> files,String path)throws IOException{File file=files.get(path);if(file==null)throw new IOException("Missing "+path+". Import this game data before exporting.");return file;}
 private static MessageDigest digest(){try{return MessageDigest.getInstance("SHA-256");}catch(NoSuchAlgorithmException e){throw new AssertionError(e);}}
 private static String hex(byte[] bytes){StringBuilder text=new StringBuilder();for(byte b:bytes)text.append(String.format(Locale.ROOT,"%02x",b&255));return text.toString();}
 private static void text(ZipOutputStream zip,String name,String value)throws IOException{ZipEntry entry=new ZipEntry(name);entry.setTime(0);zip.putNextEntry(entry);zip.write(value.getBytes("UTF-8"));zip.closeEntry();}
 static Result write(File installation,File preferredMusic,File output,String apkReceipt,BooleanSupplier cancelled,Consumer<String> progress)throws IOException{
  check(cancelled);if(installation==null||!installation.isDirectory()||Files.isSymbolicLink(installation.toPath()))throw new IOException("Import your game data first");
  File music=musicRoot(installation,preferredMusic);if(music==null)throw new IOException("No music is available. Import your music ZIP first, then export again.");
  if(Files.isSymbolicLink(music.toPath())||Files.isSymbolicLink(new File(music,"music").toPath()))throw new IOException("Music links cannot be exported");
  String destination=output.getCanonicalPath();for(File root:new File[]{installation,music})if(destination.startsWith(root.getCanonicalPath()+File.separator))throw new IOException("Export must be outside the imported game data");
  TreeMap<String,File> files=new TreeMap<>();
  // Imported snapshots keep these two files flat; ZIP import expects canonical paths.
  File base=new File(installation,"base.zwp"),shell=new File(installation,"shell.zwp");
  if(base.isFile())add(files,"packs/base.zwp",base);if(shell.isFile())add(files,"missions/system/shell.zwp",shell);
  for(String tree:TREES){File dir=new File(installation,tree);if(dir.isDirectory())walk(files,dir,tree,0);}
  File[] roots=installation.listFiles();if(roots==null)throw new IOException("Cannot read game data");
  for(File file:roots)if(file.isFile()&&file.getName().toLowerCase(Locale.ROOT).endsWith(".cfg"))add(files,file.getName().toLowerCase(Locale.ROOT),file);
  int tracks=0;for(String path:tracks(music)){File file=new File(path);String name=file.getName().toLowerCase(Locale.ROOT);if(name.matches(".*\\.(ogg|mp3|wav)")){add(files,"music/"+name,file);tracks++;}}
  if(tracks==0||tracks>128)throw new IOException("Music must contain 1–128 supported tracks");
  DataFolderImporter.validatePack(required(files,"packs/base.zwp"),"runcode_main_login.cfg","english.cfg");
  DataFolderImporter.validatePack(required(files,"missions/system/shell.zwp"),"interface_shell.cfg");
  DataFolderImporter.validatePack(required(files,"worlds/sprawl.zwp"),"types_world.cfg");
  for(String side:new String[]{"jda","sprawler"})DataFolderImporter.validatePack(required(files,"sides/client/"+side+".zwp"),"interface_game_side.cfg");
  required(files,"library/engine/startup.cfg");required(files,"missions/multiplayer/group.cfg");
  boolean maps=false,mods=false;for(String path:files.keySet()){maps|=path.startsWith("missions/multiplayer/")&&path.endsWith(".zwp");mods|=path.startsWith("mods/");}
  if(!maps||!mods)throw new IOException("Instant Action maps or AI mods are missing. Import the complete missions and mods folders first.");
  long total=0;for(Map.Entry<String,File> entry:files.entrySet()){check(cancelled);total+=entry.getValue().length();if(total>DataFolderImporter.MAX_TOTAL)throw new IOException("Game data exceeds 2 GiB");if(entry.getKey().endsWith(".zwp"))DataFolderImporter.validatePack(entry.getValue());}
  File parent=output.getAbsoluteFile().getParentFile();if(!parent.isDirectory()&&!parent.mkdirs())throw new IOException("Cannot create export directory");
  long space=parent.getUsableSpace();if(space>0&&space<total+files.size()*1024L+8L*1024*1024)throw new IOException("Not enough free storage to build the complete ZIP");
  File temporary=File.createTempFile("game-data-",".part",parent);boolean complete=false;long copied=0;
  try{
   byte[] buffer=new byte[262144];StringBuilder manifest=new StringBuilder("SHA-256\tBytes\tPath\n");
   try(ZipOutputStream zip=new ZipOutputStream(new BufferedOutputStream(new FileOutputStream(temporary)))){
    zip.setLevel(Deflater.BEST_SPEED);
    for(Map.Entry<String,File> item:files.entrySet()){
     check(cancelled);progress.accept("Exporting "+item.getKey());MessageDigest hash=digest();long count=0;ZipEntry entry=new ZipEntry(item.getKey());entry.setTime(0);zip.putNextEntry(entry);
     try(InputStream in=new FileInputStream(item.getValue())){int n;while((n=in.read(buffer))!=-1){check(cancelled);count+=n;copied+=n;if(count>DataFolderImporter.MAX_FILE||copied>DataFolderImporter.MAX_TOTAL)throw new IOException("Game data changed or exceeded export limits");zip.write(buffer,0,n);hash.update(buffer,0,n);}}
     if(count!=item.getValue().length())throw new IOException("Game data changed during export");zip.closeEntry();manifest.append(hex(hash.digest())).append('\t').append(count).append('\t').append(item.getKey()).append('\n');
    }
    text(zip,"data-file-manifest.txt",manifest.toString());text(zip,"apk-source.json",apkReceipt);
    text(zip,"README.txt","Reigncarnated single-file game data backup\n\nSelect this ZIP alone using Select game data ZIP(s).\nIncludes all currently imported runtime folders and selected music.\nSprawl, base/shell/side packs, startup config, Instant Action maps and mods were checked.\nCampaigns/worlds are included when present in the installation.\nThis archive contains your owned game data; it is separate from the APK/source.\nProfiles, saves, diagnostics and signing keys are excluded.\n");
   }
   progress.accept("Verifying ZIP checksums…");
   try(ZipFile zip=new ZipFile(temporary)){Enumeration<? extends ZipEntry> all=zip.entries();while(all.hasMoreElements()){check(cancelled);ZipEntry entry=all.nextElement();CRC32 crc=new CRC32();try(InputStream in=zip.getInputStream(entry)){int n;while((n=in.read(buffer))!=-1){check(cancelled);crc.update(buffer,0,n);}}if(crc.getValue()!=entry.getCrc())throw new IOException("Export checksum failed: "+entry.getName());}}
   MessageDigest hash=digest();try(InputStream in=new FileInputStream(temporary)){int n;while((n=in.read(buffer))!=-1){check(cancelled);hash.update(buffer,0,n);}}
   check(cancelled);Files.move(temporary.toPath(),output.toPath(),StandardCopyOption.ATOMIC_MOVE,StandardCopyOption.REPLACE_EXISTING);complete=true;
   return new Result(output,files.size(),tracks,copied,hex(hash.digest()));
  }finally{if(!complete)temporary.delete();}
 }
}
