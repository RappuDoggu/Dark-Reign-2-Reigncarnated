package org.reigncarnated.menu;
import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.zip.*;
public final class MusicImportTest {
 static int checks;
 static void check(boolean b){++checks;if(!b)throw new AssertionError("check "+checks);}
 static File zip(File root,String name,String... entries)throws Exception{File f=new File(root,name);try(ZipOutputStream z=new ZipOutputStream(new FileOutputStream(f))){for(String entry:entries){z.putNextEntry(new ZipEntry(entry));z.write(new byte[]{1,2,3,4});z.closeEntry();}}return f;}
 public static void main(String[] args)throws Exception{File root=Files.createTempDirectory("music-import-test").toFile();try{
  File archive=zip(root,"tracks.zip","original/02.ogg","original/01.mp3");File output=new File(root,"snapshots");
  try(ZipGameSource source=new ZipGameSource(Collections.singletonList(archive),false)){
   DataFolderImporter.Result r=new DataFolderImporter(source,()->false,s->{}).runMusic(source.root(),output);
   check(r.tracks==2);check(new File(r.directory,"music/000.mp3").isFile());check(Arrays.equals(Files.readAllBytes(new File(r.directory,"music/001.ogg").toPath()),new byte[]{1,2,3,4}));
   int before=output.list().length;try{new DataFolderImporter(source,()->true,s->{}).runMusic(source.root(),output);throw new AssertionError("accepted cancel");}catch(IOException expected){check(output.list().length==before);}
   final int[] calls={0};try{new DataFolderImporter(source,()->++calls[0]>5,s->{}).runMusic(source.root(),output);throw new AssertionError("accepted mid-copy cancel");}catch(IOException expected){check(output.list().length==before);}
  }
  try{new ZipGameSource(Collections.singletonList(archive));throw new AssertionError("default world check bypassed");}catch(IOException expected){check(true);}
  try{new ZipGameSource(Collections.singletonList(zip(root,"traversal.zip","../evil.ogg")),false);throw new AssertionError("accepted traversal");}catch(IOException expected){check(true);}
  try(ZipGameSource source=new ZipGameSource(Collections.singletonList(zip(root,"empty.zip","readme.txt")),false)){
   try{new DataFolderImporter(source,()->false,s->{}).runMusic(source.root(),output);throw new AssertionError("accepted empty music");}catch(IOException expected){check(true);}
  }
  String[] tooMany=new String[129];for(int i=0;i<129;i++)tooMany[i]=i+".wav";
  try(ZipGameSource source=new ZipGameSource(Collections.singletonList(zip(root,"many.zip",tooMany)),false)){
   try{new DataFolderImporter(source,()->false,s->{}).runMusic(source.root(),output);throw new AssertionError("accepted too many tracks");}catch(IOException expected){check(true);}
  }
  FrameTiming t=new FrameTiming();check(!t.frame(1,100));check(t.frame(1_000_000_000L,1_000_000_100L));check(t.frames==1&&t.max==100);t.reset();check(t.frames==0&&t.since==0);check(!t.frame(5_000_000_000L,5_000_000_100L));check(!t.frame(10_000_000_000L,10_000_000_100L));
  System.out.println("Music snapshots, rollback, ZIP safety, limits and timing: PASS ("+checks+" checks)");
 }finally{DataFolderImporter.remove(root);}}
}
