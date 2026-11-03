package org.reigncarnated.menu;
import java.util.*;
/** Render-thread state machine. Samples real frames before running intrusive checks. */
final class DiagnosticSweep {
 interface Backend {String debug(int action);String report();int[] capture();}
 interface Sink {void complete(Map<String,String> reports,Map<String,int[]> pictures);}
 private final Backend backend;private final Sink sink;private final boolean original;
 private final Map<String,String> reports=new LinkedHashMap<>();private final Map<String,int[]> pictures=new LinkedHashMap<>();
 private final List<Long> work=new ArrayList<>(),intervals=new ArrayList<>();
 private int phase,shots;private long since,previous;private boolean done;
 DiagnosticSweep(Backend b,Sink s,boolean synchronizedMode){backend=b;sink=s;original=synchronizedMode;}
 void start(){try{backend.debug(2);backend.debug(4);}catch(RuntimeException e){finish("Startup failed: "+e);}}
 private String mode(){return phase==0?"fast":"synchronized";}
 void frame(long start,long end){if(done)return;try{
  if(since==0)since=start;if(work.size()<2048)work.add(Math.max(0,end-start));
  if(previous!=0&&intervals.size()<2048)intervals.add(Math.max(0,start-previous));previous=start;
  long elapsed=end-since;
  if(shots<3&&elapsed>=shots*1_000_000_000L){int[] pixels=backend.capture();String id=mode()+"/frame-"+shots;
   reports.put(id+"-timing.txt","Callback start="+start+" end="+end+" elapsed-in-mode="+elapsed+" ns\n");if(pixels!=null&&pixels.length==640*480)pictures.put(id+".png",pixels);else reports.put(id+"-error.txt","Frame capture unavailable");++shots;
  }
  if(elapsed<2_000_000_000L||shots<3)return;
  reports.put(mode()+"/report.txt",summary(work,"native wall time")+summary(intervals,"callback interval")+"Capture readbacks occur after measured native work; callback intervals include those stalls. Not GPU elapsed time.\n"+backend.report());
  if(phase++==0){since=previous=0;shots=0;work.clear();intervals.clear();backend.debug(3);backend.debug(4);}
  else {try{reports.put("gpu-checks.txt",backend.debug(1));}catch(RuntimeException e){reports.put("gpu-checks.txt","FAILED: "+e);}finish("Completed both stream modes; three frame captures per mode requested.");}
 }catch(RuntimeException e){finish("Collection failed: "+e);}}
 private static String summary(List<Long> values,String name){if(values.isEmpty())return name+": no samples\n";List<Long> sorted=new ArrayList<>(values);Collections.sort(sorted);double sum=0;for(long n:values)sum+=n;return String.format(Locale.US,"%s: n=%d mean=%.3f p95=%.3f max=%.3f ms\n",name,values.size(),sum/values.size()/1e6,sorted.get((int)Math.ceil(sorted.size()*.95)-1)/1e6,sorted.get(sorted.size()-1)/1e6);}
 private void finish(String reason){if(done)return;done=true;try{reports.put("final-native.txt",backend.report());}catch(RuntimeException e){reports.put("final-native.txt",e.toString());}reports.put("collection.txt",reason+"\nOriginal stream mode="+(original?"synchronized":"fast")+"\nNo simulation speed or render layer changes. Samples are bounded to 2048 frames per mode.\n");try{backend.debug(original?3:2);}catch(RuntimeException e){reports.put("restore-error.txt",e.toString());}sink.complete(reports,pictures);}
 void abort(String reason){finish(reason);}
 void cancel(){if(done)return;done=true;try{backend.debug(original?3:2);}finally{pictures.clear();}}
 boolean done(){return done;}
}
