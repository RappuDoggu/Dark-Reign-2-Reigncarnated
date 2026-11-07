package org.reigncarnated.menu;
import android.app.Activity;
import android.content.*;
import android.graphics.Bitmap;
import android.net.Uri;
import android.util.Log;
import java.io.*;
import java.util.*;
import java.util.concurrent.*;
/** Bounded process journal. Disk I/O and PNG encoding never run on the GL thread. */
final class Diagnostics {
 private static final java.util.concurrent.atomic.AtomicLong dropped=new java.util.concurrent.atomic.AtomicLong();
 private static String previousTail="";
 private static final ArrayDeque<String> events=new ArrayDeque<>();
 private static final ThreadPoolExecutor writer=new ThreadPoolExecutor(1,1,0,TimeUnit.SECONDS,new ArrayBlockingQueue<>(64),r->{Thread t=new Thread(r,"ReignDiagnosticWriter");t.setDaemon(true);return t;},(r,e)->{dropped.incrementAndGet();Log.w("ReignDiag","Diagnostic writer queue full");});
 private static File session;private static String build="unknown source revision";private static boolean initialized;
 static synchronized void init(Context c){if(initialized)return;initialized=true;session=new File(c.getFilesDir(),c.getClass().getSimpleName().equals("MissionActivity")?"session-mission.log":"session.log");
  try(InputStream in=c.getAssets().open("build/source.json")){ByteArrayOutputStream b=new ByteArrayOutputStream();byte[] buf=new byte[4096];int n;while((n=in.read(buf))>0&&b.size()<16384)b.write(buf,0,n);build=b.toString("UTF-8");}catch(IOException e){Log.w("ReignDiag","No build receipt",e);}
  try(RandomAccessFile f=new RandomAccessFile(session,"r")){int size=(int)Math.min(16384,f.length());f.seek(f.length()-size);byte[] tail=new byte[size];f.readFully(tail);previousTail=new String(tail,"UTF-8");}catch(IOException ignored){}
  captureExits(c);
  Thread.UncaughtExceptionHandler prior=Thread.getDefaultUncaughtExceptionHandler();Thread.setDefaultUncaughtExceptionHandler((thread,error)->{String text="Uncaught "+thread.getName()+": "+Log.getStackTraceString(error);append(text);if(prior!=null)prior.uncaughtException(thread,error);});event("Session start "+android.os.Build.MODEL+" API "+android.os.Build.VERSION.SDK_INT);
 }
 static void saveMissionState(Context c,String mission,String report){saveMissionState(c,mission,report,false);}
 static void saveMissionFailure(Context c,String mission,String report){saveMissionState(c,mission,report,true);}
 private static void saveMissionState(Context c,String mission,String report,boolean failure){
  final String key="Mission="+mission.replace('\n',' ').replace('\r',' ').substring(0,Math.min(mission.length(),256));
  final String snapshot=key+"\nCollection timestamp="+System.currentTimeMillis()+"\n"+build+"\n"+report;final File directory=c.getFilesDir();
  writer.execute(()->{try{MissionReports.save(directory,key,snapshot,failure);}catch(IOException e){Log.w("ReignDiag","Cannot persist mission state",e);}});
 }
 static void event(String text){if(text==null)return;if(text.length()>4096)text=text.substring(0,4096);String line=System.currentTimeMillis()+" "+text; synchronized(Diagnostics.class){while(events.size()>=96)events.removeFirst();events.addLast(line);}Log.i("ReignDiag",line);final String saved=line;writer.execute(()->append(saved));}
 private static synchronized void append(String line){if(session==null)return;try{if(session.length()>512*1024){File old=new File(session.getParentFile(),session.getName()+".previous");if(old.exists()&&!old.delete())return;if(!session.renameTo(old))return;}try(FileOutputStream f=new FileOutputStream(session,true)){f.write((line+"\n").getBytes("UTF-8"));}}catch(IOException e){Log.w("ReignDiag","Cannot save session",e);}}
 private static final java.util.concurrent.atomic.AtomicBoolean exitsBusy=new java.util.concurrent.atomic.AtomicBoolean();
 private static final Set<String> seenExits=Collections.synchronizedSet(new HashSet<>());
 static void captureExits(Context context){
  if(android.os.Build.VERSION.SDK_INT<30||!exitsBusy.compareAndSet(false,true))return;final Context c=context.getApplicationContext();
  new Thread(()->{try{
   android.app.ActivityManager am=(android.app.ActivityManager)c.getSystemService(Context.ACTIVITY_SERVICE);File dir=new File(c.getFilesDir(),"crash-traces");
   for(android.app.ApplicationExitInfo exit:am.getHistoricalProcessExitReasons(c.getPackageName(),0,3)){
    String id=exit.getTimestamp()+"-"+exit.getPid();if(!seenExits.add(id))continue;if(seenExits.size()>64){seenExits.clear();seenExits.add(id);}
    event("Previous process exit: process="+exit.getProcessName()+" reason="+exit.getReason()+" status="+exit.getStatus()+" timestamp="+exit.getTimestamp()+" description="+exit.getDescription());
    int reason=exit.getReason();if(reason!=android.app.ApplicationExitInfo.REASON_CRASH_NATIVE&&reason!=android.app.ApplicationExitInfo.REASON_CRASH&&reason!=android.app.ApplicationExitInfo.REASON_ANR)continue;
    if(!dir.isDirectory()&&!dir.mkdirs())throw new IOException("Cannot create private crash-trace directory");File target=new File(dir,"exit-"+id+".trace");if(target.isFile())continue;File temporary=new File(dir,"exit-"+id+".tmp");
    try(InputStream in=exit.getTraceInputStream()){
     if(in==null){event("No platform trace available for "+id);continue;}try(FileOutputStream out=new FileOutputStream(temporary)){byte[] b=new byte[8192];int total=0,n;while(total<262144&&(n=in.read(b,0,Math.min(b.length,262144-total)))!=-1){out.write(b,0,n);total+=n;}event("Captured bounded platform crash trace "+id+" bytes="+total);}
     if(!temporary.renameTo(target))throw new IOException("Cannot publish private crash trace");
    }catch(IOException|RuntimeException e){temporary.delete();event("Crash trace unavailable: "+e);}
   }
   File[] traces=dir.listFiles((d,n)->n.matches("exit-[0-9]+-[0-9]+\\.trace"));if(traces!=null){Arrays.sort(traces,(a,b)->Long.compare(b.lastModified(),a.lastModified()));for(int i=3;i<traces.length;i++)traces[i].delete();}
  }catch(IOException|RuntimeException e){event("Exit diagnostics unavailable: "+e);}finally{exitsBusy.set(false);}},"ReignExitTrace").start();
 }
 private static void addTail(Map<String,byte[]> entries,String name,File file,int limit)throws IOException{if(!file.isFile())return;try(RandomAccessFile in=new RandomAccessFile(file,"r")){int size=(int)Math.min(limit,in.length());in.seek(in.length()-size);byte[] bytes=new byte[size];in.readFully(bytes);entries.put(name,bytes);}}
 static synchronized String report(String latest){StringBuilder b=new StringBuilder("Reigncarnated recovery\n").append(build).append('\n').append(latest).append("\nPrevious tail:\n").append(previousTail).append("\nDropped writer tasks=").append(dropped.get()).append("\nSession events:\n");for(String e:events)b.append(e).append('\n');return b.toString();}
 static void share(Activity a,String latest,int[] pixels){final String text=report(latest);writer.execute(()->{try{
  File dir=new File(a.getCacheDir(),"diagnostic-share");if(!dir.isDirectory()&&!dir.mkdirs())throw new IOException("Cannot create diagnostic share");
  try(FileOutputStream f=new FileOutputStream(new File(dir,"report.txt"))){f.write(text.getBytes("UTF-8"));}
  ArrayList<Uri> uris=new ArrayList<>();uris.add(Uri.parse("content://"+a.getPackageName()+".diagnostics/report.txt"));
  if(pixels!=null){if(pixels.length!=640*480)throw new IOException("Unexpected capture size");Bitmap bitmap=Bitmap.createBitmap(pixels,640,480,Bitmap.Config.ARGB_8888);try(FileOutputStream f=new FileOutputStream(new File(dir,"capture.png"))){if(!bitmap.compress(Bitmap.CompressFormat.PNG,100,f))throw new IOException("PNG encoding failed");}finally{bitmap.recycle();}uris.add(Uri.parse("content://"+a.getPackageName()+".diagnostics/capture.png"));}
  a.runOnUiThread(()->{if(a.isFinishing())return;Intent i=new Intent(Intent.ACTION_SEND_MULTIPLE).setType(pixels==null?"text/plain":"*/*");i.putParcelableArrayListExtra(Intent.EXTRA_STREAM,uris);i.putExtra(Intent.EXTRA_TEXT,text);ClipData clip=ClipData.newRawUri("diagnostics",uris.get(0));for(int n=1;n<uris.size();n++)clip.addItem(new ClipData.Item(uris.get(n)));i.setClipData(clip);i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);a.startActivity(Intent.createChooser(i,"Share diagnostics"));});
 }catch(Exception e){event("Share failed: "+e);}});}
 private static final java.util.concurrent.atomic.AtomicBoolean zipBusy=new java.util.concurrent.atomic.AtomicBoolean();
 static void shareZip(Activity a,Map<String,String> reports,Map<String,int[]> pictures){
  if(!zipBusy.compareAndSet(false,true)){a.runOnUiThread(()->android.widget.Toast.makeText(a,"Diagnostic ZIP is already being prepared",android.widget.Toast.LENGTH_LONG).show());return;}
  final String sessionReport=report("Collection timestamp="+System.currentTimeMillis());
  new Thread(()->{try{
   Map<String,byte[]> entries=new LinkedHashMap<>();entries.put("session.txt",sessionReport.getBytes("UTF-8"));entries.put("source.json",build.getBytes("UTF-8"));
   addTail(entries,"last-mission-report.txt",new File(a.getFilesDir(),"last-mission-report.txt"),128*1024);
   addTail(entries,"previous-mission-report.txt",new File(a.getFilesDir(),"previous-mission-report.txt"),128*1024);
   for(String name:new String[]{"last-mission-failure.txt","previous-mission-failure.txt"})addTail(entries,name,new File(a.getFilesDir(),name),128*1024);
   addTail(entries,"last-mission-loading.log",new File(a.getFilesDir(),"mission-runtime/loading-last.log"),100*1024);
   addTail(entries,"mission-process.log",new File(a.getFilesDir(),"session-mission.log"),256*1024);
   File[] traces=new File(a.getFilesDir(),"crash-traces").listFiles((d,n)->n.matches("exit-[0-9]+-[0-9]+\\.trace"));if(traces!=null){Arrays.sort(traces,(x,y)->Long.compare(y.lastModified(),x.lastModified()));for(int i=0;i<Math.min(3,traces.length);i++)addTail(entries,"crashes/"+traces[i].getName(),traces[i],262144);}
   for(Map.Entry<String,String> entry:reports.entrySet())entries.put(entry.getKey(),entry.getValue().getBytes("UTF-8"));
   for(Map.Entry<String,int[]> entry:pictures.entrySet()){Bitmap bitmap=Bitmap.createBitmap(entry.getValue(),640,480,Bitmap.Config.ARGB_8888);try(ByteArrayOutputStream bytes=new ByteArrayOutputStream()){if(!bitmap.compress(Bitmap.CompressFormat.PNG,100,bytes))throw new IOException("Cannot encode frame");entries.put(entry.getKey(),bytes.toByteArray());}finally{bitmap.recycle();}}
   synchronized(Diagnostics.class){if(session!=null&&session.isFile())try(RandomAccessFile f=new RandomAccessFile(session,"r")){int size=(int)Math.min(512*1024,f.length());f.seek(f.length()-size);byte[] tail=new byte[size];f.readFully(tail);entries.put("session-tail.log",tail);}}
   File dir=new File(a.getCacheDir(),"diagnostic-share");if(!dir.isDirectory()&&!dir.mkdirs())throw new IOException("Cannot create diagnostic share");String stage=a.getClass().getSimpleName().equals("MenuActivity")?"menu":"mission";
   entries.put("screen.txt",(stage+"\nPrevious other-screen collection, if included, has its own source/time receipt.\n").getBytes("UTF-8"));
   DiagnosticZip.write(new File(dir,stage+"-last.zip"),entries);
   File previous=new File(dir,(stage.equals("menu")?"mission":"menu")+"-last.zip");
   if(previous.isFile()&&previous.length()<=6L*1024*1024)entries.put("previous-"+previous.getName(),java.nio.file.Files.readAllBytes(previous.toPath()));
   else entries.put("other-screen.txt",(previous.isFile()?"Previous other-screen collection omitted: exceeds 6 MiB cap":"No earlier collection from the other screen; collect once in menu and once in mission").getBytes("UTF-8"));
   DiagnosticZip.write(new File(dir,"diagnostics.zip"),entries);
   event("Diagnostic ZIP ready; files="+entries.size());a.runOnUiThread(()->{if(a.isFinishing())return;Uri uri=Uri.parse("content://"+a.getPackageName()+".diagnostics/diagnostics.zip");Intent i=new Intent(Intent.ACTION_SEND).setType("application/zip");i.putExtra(Intent.EXTRA_STREAM,uri);i.setClipData(ClipData.newRawUri("diagnostics.zip",uri));i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);new android.app.AlertDialog.Builder(a).setTitle("Diagnostics ZIP ready").setMessage("Save diagnostics.zip, then attach it to this chat. It contains screenshots and diagnostic logs.").setPositiveButton("Save ZIP",(d,w)->a.startActivityForResult(new Intent(Intent.ACTION_CREATE_DOCUMENT).setType("application/zip").addCategory(Intent.CATEGORY_OPENABLE).putExtra(Intent.EXTRA_TITLE,"Reigncarnated-diagnostics.zip"),9315)).setNeutralButton("Share ZIP",(d,w)->a.startActivity(Intent.createChooser(i,"Share diagnostics.zip"))).setNegativeButton("Close",null).show();});
  }catch(Exception e){event("Diagnostic ZIP failed: "+e);a.runOnUiThread(()->android.widget.Toast.makeText(a,"Diagnostic ZIP failed: "+e.getMessage(),android.widget.Toast.LENGTH_LONG).show());}finally{pictures.clear();zipBusy.set(false);}},"ReignDiagnosticZip").start();
 }

 static boolean saveResult(Activity a,int request,int result,Intent data){if(request!=9315)return false;if(result!=Activity.RESULT_OK||data==null||data.getData()==null)return true;final Uri destination=data.getData();
  new Thread(()->{try(InputStream in=new FileInputStream(new File(a.getCacheDir(),"diagnostic-share/diagnostics.zip"));OutputStream out=a.getContentResolver().openOutputStream(destination,"wt")){if(out==null)throw new IOException("Cannot open destination");byte[] buffer=new byte[65536];int n;while((n=in.read(buffer))!=-1)out.write(buffer,0,n);a.runOnUiThread(()->android.widget.Toast.makeText(a,"Diagnostics ZIP saved. Attach it to this chat.",android.widget.Toast.LENGTH_LONG).show());}catch(Exception e){event("Diagnostic save failed: "+e);a.runOnUiThread(()->android.widget.Toast.makeText(a,"ZIP save failed: "+e.getMessage(),android.widget.Toast.LENGTH_LONG).show());}},"ReignSaveDiagnostics").start();return true;
 }

}
