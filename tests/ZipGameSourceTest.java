package org.reigncarnated.menu;
import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.zip.*;
public final class ZipGameSourceTest {
 static int checks;
 static void check(boolean ok){checks++;if(!ok)throw new AssertionError("ZIP check "+checks);}
 static File zip(File dir,String name,String[] paths,byte[][] bytes)throws Exception{
  File file=new File(dir,name);try(ZipOutputStream out=new ZipOutputStream(new FileOutputStream(file))){for(int i=0;i<paths.length;i++){out.putNextEntry(new ZipEntry(paths[i]));out.write(bytes[i]);out.closeEntry();}}return file;
 }
 static boolean fails(List<File> files,File parent)throws Exception{try(ZipGameSource source=new ZipGameSource(files)){new DataFolderImporter(source,()->false,s->{}).run(source.root(),parent);return false;}catch(IOException expected){return true;}}
 public static void main(String[] args)throws Exception{
  File dir=Files.createTempDirectory("reign-zip-test").toFile(),dest=new File(dir,"imports");dest.mkdir();
  byte[] base=DataFolderImporterTest.pack("runcode_main_login.cfg","english.cfg"),shell=DataFolderImporterTest.pack("interface_shell.cfg"),world=DataFolderImporterTest.pack("types_world.cfg");
  try{
   File first=zip(dir,"menu.zip",new String[]{"DR2/PACKS/BASE.ZWP","DR2/missions/system/shell.zwp","DR2/Music/Track02.ogg"},new byte[][]{base,shell,new byte[]{1,2,3}});
   File second=zip(dir,"world.zip",new String[]{"sprawl.zwp"},new byte[][]{world});
   File kept;
   try(ZipGameSource source=new ZipGameSource(Arrays.asList(first,second))){DataFolderImporter.Result r=new DataFolderImporter(source,()->false,s->{}).run(source.root(),dest);check(r.tracks==1&&r.scenePacks==1);kept=r.directory;check(Arrays.equals(base,Files.readAllBytes(new File(kept,"base.zwp").toPath())));}
   check(fails(Arrays.asList(first),dest)); // explicit missing world
   check(fails(Arrays.asList(first,second,second),dest)); // ambiguous multi-ZIP contents
   File bad=zip(dir,"bad.zip",new String[]{"../sprawl.zwp"},new byte[][]{world});check(fails(Arrays.asList(first,bad),dest));
   File corrupt=zip(dir,"corrupt.zip",new String[]{"base.zwp","shell.zwp","sprawl.zwp"},new byte[][]{new byte[]{1},shell,world});check(fails(Arrays.asList(corrupt),dest));
   try(ZipGameSource source=new ZipGameSource(Arrays.asList(first,second))){boolean cancelled=false;try{new DataFolderImporter(source,()->true,s->{}).run(source.root(),dest);}catch(IOException e){cancelled=true;}check(cancelled);}
   check(dest.list().length==1);check(Arrays.equals(base,Files.readAllBytes(new File(kept,"base.zwp").toPath())));
   if(args.length>0)try(ZipGameSource source=new ZipGameSource(Collections.singletonList(new File(args[0])))){DataFolderImporter.Result r=new DataFolderImporter(source,()->false,s->{}).run(source.root(),dest);check(r.tracks==9&&r.scenePacks>=1);System.out.println("Real game ZIP: "+r.tracks+" music tracks, "+r.scenePacks+" scene packs");if(args.length>1){File target=new File(args[1]);if(target.exists()||!r.directory.renameTo(target))throw new IOException("Cannot retain real import for GPU check");}}
   System.out.println("ZIP import, multiple archives, missing world, duplicates, traversal, rollback and cancellation: PASS ("+checks+")");
  }finally{DataFolderImporter.remove(dir);}
 }
}
