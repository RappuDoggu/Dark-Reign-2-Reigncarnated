package org.reigncarnated.menu;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;

/** Bounded private reports; successful missions never replace failure evidence. */
final class MissionReports {
 static final int LIMIT=128*1024;
 static void save(File directory,String key,String snapshot,boolean failure)throws IOException{
  pair(directory,"last-mission-report.txt","previous-mission-report.txt",key,snapshot);
  if(failure)pair(directory,"last-mission-failure.txt","previous-mission-failure.txt",key,snapshot);
 }
 private static void pair(File dir,String last,String previous,String key,String snapshot)throws IOException{
  File target=new File(dir,last);
  if(target.isFile()){
   String old;try(BufferedReader in=new BufferedReader(new InputStreamReader(new FileInputStream(target),StandardCharsets.UTF_8))){old=in.readLine();}
   if(!key.equals(old))Files.copy(target.toPath(),new File(dir,previous).toPath(),StandardCopyOption.REPLACE_EXISTING);
  }
  byte[] data=snapshot.getBytes(StandardCharsets.UTF_8);File temporary=new File(dir,last+".tmp");
  try(FileOutputStream out=new FileOutputStream(temporary)){out.write(data,0,Math.min(data.length,LIMIT));}
  if(!temporary.renameTo(target))throw new IOException("Cannot publish private mission report");
 }
}
