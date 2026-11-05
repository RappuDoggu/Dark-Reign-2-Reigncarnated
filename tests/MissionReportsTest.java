package org.reigncarnated.menu;
import java.io.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
public final class MissionReportsTest {
 static int checks;
 static void check(boolean ok){checks++;if(!ok)throw new AssertionError("report check "+checks);}
 static String read(File dir,String name)throws IOException{return new String(Files.readAllBytes(new File(dir,name).toPath()),StandardCharsets.UTF_8);}
 static void save(File dir,String mission,String receipt,boolean fail)throws IOException{String key="Mission="+mission;MissionReports.save(dir,key,key+"\nsource="+receipt+"\n"+(fail?"FAILED VarPtr caller module+0x123":"READY"),fail);}
 public static void main(String[] args)throws Exception{
  File dir=Files.createTempDirectory("reign-mission-reports-").toFile();
  try{
   save(dir,"J01","revision-one",true);check(read(dir,"last-mission-failure.txt").contains("revision-one"));
   save(dir,"J01","revision-one",false);check(!new File(dir,"previous-mission-report.txt").exists());check(read(dir,"last-mission-failure.txt").contains("FAILED"));
   save(dir,"S01_Intro","revision-two",false);save(dir,"2siege","revision-two",false);
   check(read(dir,"last-mission-report.txt").startsWith("Mission=2siege\n"));check(read(dir,"previous-mission-report.txt").startsWith("Mission=S01_Intro\n"));check(read(dir,"last-mission-failure.txt").contains("J01\nsource=revision-one\nFAILED VarPtr caller"));
   save(dir,"S01","revision-three",true);check(read(dir,"previous-mission-failure.txt").contains("J01\nsource=revision-one"));check(read(dir,"last-mission-failure.txt").contains("S01\nsource=revision-three"));
   save(dir,"S01","revision-four",true);check(read(dir,"previous-mission-failure.txt").contains("J01\nsource=revision-one"));
   String key="Mission=big";MissionReports.save(dir,key,key+"\n"+new String(new char[200000]).replace('\0','x'),true);check(new File(dir,"last-mission-report.txt").length()==MissionReports.LIMIT);check(new File(dir,"last-mission-failure.txt").length()==MissionReports.LIMIT);
   check(read(dir,"previous-mission-failure.txt").contains("S01\nsource=revision-four"));check(!new File(dir,"last-mission-report.txt.tmp").exists());
   System.out.println("Current/failed mission separation, two distinct failures surviving successful missions, provenance, bounds and publication PASS ("+checks+" checks)");
  }finally{for(File file:dir.listFiles())file.delete();dir.delete();}
 }
}
