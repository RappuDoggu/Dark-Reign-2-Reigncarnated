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
 static synchronized void init(Context c){if(initialized)return;initialized=true;session=new File(c.getFilesDir(),"session.log");
  try(InputStream in=c.getAssets().open("build/source.json")){ByteArrayOutputStream b=new ByteArrayOutputStream();byte[] buf=new byte[4096];int n;while((n=in.read(buf))>0&&b.size()<16384)b.write(buf,0,n);build=b.toString("UTF-8");}catch(IOException e){Log.w("ReignDiag","No build receipt",e);}
  try(RandomAccessFile f=new RandomAccessFile(session,"r")){int size=(int)Math.min(16384,f.length());f.seek(f.length()-size);byte[] tail=new byte[size];f.readFully(tail);previousTail=new String(tail,"UTF-8");}catch(IOException ignored){}
  if(android.os.Build.VERSION.SDK_INT>=30){android.app.ActivityManager am=(android.app.ActivityManager)c.getSystemService(Context.ACTIVITY_SERVICE);try{for(android.app.ApplicationExitInfo exit:am.getHistoricalProcessExitReasons(c.getPackageName(),0,3))event("Previous process exit: reason="+exit.getReason()+" timestamp="+exit.getTimestamp()+" description="+exit.getDescription());}catch(RuntimeException e){Log.w("ReignDiag","Exit reasons unavailable",e);}}
  Thread.UncaughtExceptionHandler prior=Thread.getDefaultUncaughtExceptionHandler();Thread.setDefaultUncaughtExceptionHandler((thread,error)->{String text="Uncaught "+thread.getName()+": "+Log.getStackTraceString(error);append(text);if(prior!=null)prior.uncaughtException(thread,error);});event("Session start "+android.os.Build.MODEL+" API "+android.os.Build.VERSION.SDK_INT);
 }
 static void event(String text){if(text==null)return;if(text.length()>4096)text=text.substring(0,4096);String line=System.currentTimeMillis()+" "+text; synchronized(Diagnostics.class){while(events.size()>=96)events.removeFirst();events.addLast(line);}Log.i("ReignDiag",line);final String saved=line;writer.execute(()->append(saved));}
 private static synchronized void append(String line){if(session==null)return;try{if(session.length()>512*1024){File old=new File(session.getParentFile(),"session.previous.log");if(old.exists()&&!old.delete())return;if(!session.renameTo(old))return;}try(FileOutputStream f=new FileOutputStream(session,true)){f.write((line+"\n").getBytes("UTF-8"));}}catch(IOException e){Log.w("ReignDiag","Cannot save session",e);}}
 static synchronized String report(String latest){StringBuilder b=new StringBuilder("Reigncarnated recovery\n").append(build).append('\n').append(latest).append("\nPrevious tail:\n").append(previousTail).append("\nDropped writer tasks=").append(dropped.get()).append("\nSession events:\n");for(String e:events)b.append(e).append('\n');return b.toString();}
 static void share(Activity a,String latest,int[] pixels){final String text=report(latest);writer.execute(()->{try{
  File dir=new File(a.getCacheDir(),"diagnostic-share");if(!dir.isDirectory()&&!dir.mkdirs())throw new IOException("Cannot create diagnostic share");
  try(FileOutputStream f=new FileOutputStream(new File(dir,"report.txt"))){f.write(text.getBytes("UTF-8"));}
  ArrayList<Uri> uris=new ArrayList<>();uris.add(Uri.parse("content://"+a.getPackageName()+".diagnostics/report.txt"));
  if(pixels!=null){if(pixels.length!=640*480)throw new IOException("Unexpected capture size");Bitmap bitmap=Bitmap.createBitmap(pixels,640,480,Bitmap.Config.ARGB_8888);try(FileOutputStream f=new FileOutputStream(new File(dir,"capture.png"))){if(!bitmap.compress(Bitmap.CompressFormat.PNG,100,f))throw new IOException("PNG encoding failed");}finally{bitmap.recycle();}uris.add(Uri.parse("content://"+a.getPackageName()+".diagnostics/capture.png"));}
  a.runOnUiThread(()->{if(a.isFinishing())return;Intent i=new Intent(Intent.ACTION_SEND_MULTIPLE).setType(pixels==null?"text/plain":"*/*");i.putParcelableArrayListExtra(Intent.EXTRA_STREAM,uris);i.putExtra(Intent.EXTRA_TEXT,text);ClipData clip=ClipData.newRawUri("diagnostics",uris.get(0));for(int n=1;n<uris.size();n++)clip.addItem(new ClipData.Item(uris.get(n)));i.setClipData(clip);i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);a.startActivity(Intent.createChooser(i,"Share diagnostics"));});
 }catch(Exception e){event("Share failed: "+e);}});}
}
