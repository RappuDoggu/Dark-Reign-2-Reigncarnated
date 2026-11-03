package org.reigncarnated.menu;
import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.zip.*;
public final class DiagnosticSweepTest {
 static int checks;static void check(boolean b){checks++;if(!b)throw new AssertionError("check "+checks);}
 static final class Backend implements DiagnosticSweep.Backend {
  List<Integer> actions=new ArrayList<>();int captures;boolean fail;
  public String debug(int action){actions.add(action);return action==1?"GPU checks PASS":"mode changed";}
  public String report(){return "native journal and audit";}
  public int[] capture(){if(fail)throw new IllegalStateException("readback failed");captures++;return new int[640*480];}
 }
 public static void main(String[] args)throws Exception{
  Backend b=new Backend();final int[] completed={0};
  DiagnosticSweep sweep=new DiagnosticSweep(b,(reports,pictures)->{completed[0]++;check(pictures.size()==6);check(reports.containsKey("fast/report.txt"));check(reports.containsKey("synchronized/report.txt"));check(reports.get("gpu-checks.txt").contains("PASS"));check(reports.containsKey("final-native.txt"));},false);
  sweep.start();for(int frame=0;frame<12;frame++){long t=1+frame*500_000_000L;sweep.frame(t,t+1_000_000);}
  check(sweep.done()&&completed[0]==1);check(b.captures==6);check(b.actions.equals(Arrays.asList(2,4,3,4,1,2)));sweep.frame(9,10);check(completed[0]==1);
  Backend cancelled=new Backend();DiagnosticSweep cancel=new DiagnosticSweep(cancelled,(r,p)->{throw new AssertionError("shared cancelled capture");},true);cancel.start();cancel.cancel();check(cancelled.actions.equals(Arrays.asList(2,4,3)));cancel.frame(1,2);check(cancelled.captures==0);
  Backend failing=new Backend();failing.fail=true;DiagnosticSweep fail=new DiagnosticSweep(failing,(r,p)->{check(r.get("collection.txt").contains("failed"));check(r.containsKey("final-native.txt"));},true);fail.start();fail.frame(1,2);check(fail.done());check(failing.actions.get(failing.actions.size()-1)==3);
  File dir=Files.createTempDirectory("diagnostic-zip-test").toFile();try{
   File zip=new File(dir,"diagnostics.zip");Map<String,byte[]> entries=new LinkedHashMap<>();entries.put("fast/report.txt","report".getBytes("UTF-8"));entries.put("source.json","{}".getBytes("UTF-8"));DiagnosticZip.write(zip,entries);
   try(ZipFile z=new ZipFile(zip)){check(z.size()==2);check(z.getEntry("fast/report.txt")!=null);check(z.getEntry("source.json")!=null);}
   byte[] previous=Files.readAllBytes(zip.toPath());entries.put("../secret",new byte[1]);try{DiagnosticZip.write(zip,entries);throw new AssertionError("unsafe path accepted");}catch(IOException expected){check(Arrays.equals(previous,Files.readAllBytes(zip.toPath())));check(!new File(dir,"diagnostics.zip.part").exists());}
   entries.remove("../secret");entries.put("large.txt",new byte[16*1024*1024]);try{DiagnosticZip.write(zip,entries);throw new AssertionError("oversize accepted");}catch(IOException expected){check(Arrays.equals(previous,Files.readAllBytes(zip.toPath())));}
  }finally{DataFolderImporter.remove(dir);}
  System.out.println("Diagnostic sweep modes, captures, restoration, failure and ZIP publication: PASS ("+checks+" checks)");
 }
}
