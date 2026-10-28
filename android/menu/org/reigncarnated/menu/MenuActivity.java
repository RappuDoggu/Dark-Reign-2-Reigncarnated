package org.reigncarnated.menu;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.opengl.GLSurfaceView;
import android.view.MotionEvent;
import android.view.KeyEvent;
import android.view.View;
import android.view.Gravity;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.text.InputType;
import android.widget.*;
import java.io.*;
import android.net.Uri;
import java.util.*;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public final class MenuActivity extends Activity {
 static {System.loadLibrary("reign_menu");}
 private static final int PICK_FOLDER=20,PICK_ZIP=21;
 private native long openNative(String pack,String profiles,String shell);
 private native String errorNative();
 private native String drawNative(long h,int w,int height,long ms);
 private native String inputNative(long h,int kind,int x,int y,String text);
 private native String resetGpuNative(long h);
 private native void closeNative(long h);
 private native long[] checkpointNative(long h);
 private native String restoreNative(long h,long clip,long elapsed,long now);
 private volatile long[] launchCheckpoint;
 private volatile int audioGeneration;
 private native String[] audioNative(long h);
 private native String ambientNative(long h);
 private native int[] movieInfoNative(long h);
 private native short[] moviePcmNative(long h);
 private IntroAudioPlayer introAudio;private boolean menuReady;
 private native float volumeNative(long h);
 private volatile MenuAudioPlayer audioPlayer;
 private MenuView view;private LinearLayout setup;private TextView status;private Button choose,chooseZip,cancelImport;
 private String report="",dataRoot;private volatile boolean importing,cancelled,destroyed;
 private volatile boolean resumed;
 private final StringBuilder history=new StringBuilder();
 private int dp(int n){return Math.round(n*getResources().getDisplayMetrics().density);}
 private boolean hasData(){String base=getPreferences(0).getString("pack",null),shell=getPreferences(0).getString("shell",null);return base!=null&&shell!=null&&new File(base).isFile()&&new File(shell).isFile()&&dataRoot!=null&&new File(dataRoot,"worlds/sprawl.zwp").isFile();}
 private void record(String s){runOnUiThread(()->{if(destroyed)return;report=s;if(history.length()>12000)history.delete(0,6000);history.append(s).append('\n');});}
 private void message(String s){if(s==null)return;runOnUiThread(()->{
  if(destroyed)return;if(s.equals("@quit")){finish();return;}if(s.equals("@options")){showOptions();return;}
  record(s);if(setup.getVisibility()==View.VISIBLE)status.setText(s);else Toast.makeText(this,s,Toast.LENGTH_LONG).show();
 });}
 private void startupFailure(String s){runOnUiThread(()->{if(destroyed)return;setup.setVisibility(View.VISIBLE);message(s);});}
 @Override public void onCreate(Bundle saved){
  super.onCreate(saved);dataRoot=getPreferences(0).getString("dataRoot",null);
  FrameLayout screen=new FrameLayout(this);view=new MenuView();screen.addView(view,new FrameLayout.LayoutParams(-1,-1));
  Button reportButton=new Button(this);reportButton.setText("Report / data");reportButton.setOnClickListener(v->new android.app.AlertDialog.Builder(this).setTitle("Native menu proof of concept").setMessage("Original intros, native 3D menu and atmospheric effects. Menu navigation is deferred.").setPositiveButton("Share report",(d,w)->{Intent i=new Intent(Intent.ACTION_SEND);i.setType("text/plain");i.putExtra(Intent.EXTRA_TEXT,"Reigncarnated 0.5 Native Launch\n"+history.toString());startActivity(Intent.createChooser(i,"Share report"));}).setNeutralButton("Import ZIP",(d,w)->pickZip()).setNegativeButton("Close",null).show());
  FrameLayout.LayoutParams tools=new FrameLayout.LayoutParams(-2,-2,Gravity.TOP|Gravity.END);screen.addView(reportButton,tools);
  setup=new LinearLayout(this);setup.setOrientation(LinearLayout.VERTICAL);setup.setGravity(Gravity.CENTER);setup.setPadding(dp(28),dp(24),dp(28),dp(24));setup.setBackgroundColor(0xff101710);
  TextView title=new TextView(this);title.setText("REIGNCARNATED");title.setTextColor(0xffaadd99);title.setTextSize(28);title.setGravity(Gravity.CENTER);setup.addView(title);
  TextView description=new TextView(this);description.setText("Select ZIP files from your legally owned Dark Reign 2 installation.\n\nRequired: base.zwp, shell.zwp and sprawl.zwp. Music is imported automatically. Select multiple ZIPs together if needed. Data is copied once; later launches play the original introduction and open the native menu.");description.setTextColor(0xffeeeeee);description.setGravity(Gravity.CENTER);description.setPadding(0,dp(20),0,dp(20));setup.addView(description);
  chooseZip=new Button(this);chooseZip.setText("Select game data ZIP(s)");chooseZip.setOnClickListener(v->pickZip());setup.addView(chooseZip);
  choose=new Button(this);choose.setText("Select game data folder");choose.setOnClickListener(v->pickFolder());setup.addView(choose);
  cancelImport=new Button(this);cancelImport.setText("Cancel import");cancelImport.setVisibility(View.GONE);cancelImport.setOnClickListener(v->{cancelled=true;status.setText("Cancelling import…");});setup.addView(cancelImport);
  Button back=new Button(this);back.setText("Return to existing game");back.setVisibility(hasData()?View.VISIBLE:View.GONE);back.setOnClickListener(v->{if(!importing)setup.setVisibility(View.GONE);});setup.addView(back);
  status=new TextView(this);status.setTextColor(0xffdddddd);status.setGravity(Gravity.CENTER);status.setPadding(0,dp(16),0,0);setup.addView(status);
  screen.addView(setup,new FrameLayout.LayoutParams(-1,-1));setup.setVisibility(hasData()?View.GONE:View.VISIBLE);setContentView(screen);
  // Delete only unreferenced importer-owned snapshots, captured before a new import can start.
  final String keepRoot=dataRoot;File parent=new File(getFilesDir(),"installations");File[] abandoned=parent.listFiles();
  if(abandoned!=null)new Thread(()->{for(File f:abandoned)if((f.getName().startsWith("import-")||f.getName().startsWith("data-"))&&!f.getAbsolutePath().equals(keepRoot))DataFolderImporter.remove(f);},"ReignCleanup").start();
 }
 private void showKeyboard(){view.requestFocus();((InputMethodManager)getSystemService(INPUT_METHOD_SERVICE)).showSoftInput(view,InputMethodManager.SHOW_IMPLICIT);}
 private void pickZip(){if(importing)return;Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT);i.addCategory(Intent.CATEGORY_OPENABLE);i.setType("*/*");i.putExtra(Intent.EXTRA_ALLOW_MULTIPLE,true);i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);startActivityForResult(i,PICK_ZIP);}
 private void pickFolder(){if(importing)return;Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);startActivityForResult(i,PICK_FOLDER);}
 private List<String> musicTracks(){if(dataRoot!=null)return DataFolderImporter.playlist(new File(dataRoot));String legacy=getPreferences(0).getString("music",null);return legacy!=null&&new File(legacy).isFile()?Collections.singletonList(legacy):Collections.emptyList();}
 private void startAudio(){if(!resumed)return;if(audioPlayer!=null)audioPlayer.close();audioPlayer=new MenuAudioPlayer(this,this::message);audioPlayer.setMusicVolume(getPreferences(0).getInt("musicVolume",40)/100f);if(menuReady)audioPlayer.playPlaylist(musicTracks());}
 @Override protected void onResume(){super.onResume();resumed=true;menuReady=false;startAudio();view.onResume();view.requestRender();}
 @Override protected void onPause(){resumed=false;audioGeneration++;menuReady=false;if(introAudio!=null){introAudio.close();introAudio=null;}MenuAudioPlayer old=audioPlayer;audioPlayer=null;if(old!=null)old.close();view.queueEvent(()->{if(view.handle!=0){launchCheckpoint=checkpointNative(view.handle);closeNative(view.handle);view.handle=0;}});view.onPause();super.onPause();}
 @Override protected void onDestroy(){destroyed=true;cancelled=true;super.onDestroy();}
 private final class MenuView extends GLSurfaceView implements GLSurfaceView.Renderer {
  long handle;int width,height;String pack,shell;boolean failed;String gpu="";
  MenuView(){super(MenuActivity.this);pack=getPreferences(0).getString("pack",null);shell=getPreferences(0).getString("shell",null);if(!hasData())pack=null;setFocusable(true);setFocusableInTouchMode(true);setEGLContextClientVersion(3);setEGLConfigChooser(8,8,8,8,16,0);setRenderer(this);setRenderMode(RENDERMODE_CONTINUOUSLY);}
  public void onSurfaceCreated(GL10 gl,EGLConfig c){gpu="Device: "+android.os.Build.MANUFACTURER+" "+android.os.Build.MODEL+" API "+android.os.Build.VERSION.SDK_INT+"\nGPU: "+gl.glGetString(GL10.GL_RENDERER);failed=false;if(handle!=0){String error=resetGpuNative(handle);if(error!=null){failed=true;message(error);}}}
  public void onSurfaceChanged(GL10 gl,int w,int h){width=w;height=h;requestRender();}
  public void onDrawFrame(GL10 gl){final int generation=audioGeneration;if(!resumed)return;if(width<=0||height<=0||pack==null||failed||importing)return;if(handle==0){handle=openNative(pack,new File(getFilesDir(),"profiles").getAbsolutePath(),shell);if(handle==0){failed=true;startupFailure("Native startup failed: "+errorNative());return;}long[] checkpoint=launchCheckpoint;if(checkpoint!=null){String restoreError=restoreNative(handle,checkpoint[0],checkpoint[1],android.os.SystemClock.uptimeMillis());if(restoreError!=null){failed=true;startupFailure(restoreError);return;}launchCheckpoint=null;}record(gpu+"\nOriginal intro sequence loaded. Native shell follows the movies; tap to skip the current clip.");}String e=drawNative(handle,width,height,android.os.SystemClock.uptimeMillis());String ambientPath=ambientNative(handle);int[] movie=movieInfoNative(handle);short[] pcm=moviePcmNative(handle);
   runOnUiThread(()->{if(destroyed||!resumed||generation!=audioGeneration)return;try{if(movie!=null&&movie[0]>0){if(movie[1]>0){if(introAudio==null||introAudio.movie!=movie[0]){if(introAudio!=null)introAudio.close();introAudio=new IntroAudioPlayer(MenuActivity.this,movie[0],movie[1],MenuActivity.this::message);}introAudio.accept(pcm);}}else if(!menuReady){if(introAudio!=null){introAudio.close();introAudio=null;}menuReady=true;startAudio();if(audioPlayer!=null)audioPlayer.playAmbient(ambientPath);}}catch(Exception ex){message("Intro audio failed: "+ex.getMessage());}});
   String[] sounds=audioNative(handle);MenuAudioPlayer player=audioPlayer;if(player!=null&&sounds!=null&&sounds.length>0)runOnUiThread(()->{if(audioPlayer==player)player.accept(sounds);});if(e!=null){failed=true;startupFailure("Render failure: "+e);}}
  void input(int kind,int x,int y,String text){queueEvent(()->{if(handle!=0)message(inputNative(handle,kind,x,y,text));});requestRender();}
  @Override public boolean onTouchEvent(MotionEvent e){requestFocus();if(e.getPointerCount()!=1){input(2,-32768,-32768,null);return true;}float scale=Math.min(width/640f,height/480f);if(scale<=0)return true;int x=(int)((e.getX()-(width-640*scale)/2)/scale),y=(int)((e.getY()-(height-480*scale)/2)/scale);int a=e.getActionMasked();if(a==MotionEvent.ACTION_DOWN)input(1,x,y,null);else if(a==MotionEvent.ACTION_UP)input(2,x,y,null);else if(a==MotionEvent.ACTION_CANCEL)input(2,-32768,-32768,null);else if(a==MotionEvent.ACTION_MOVE)input(0,x,y,null);return true;}
  @Override public boolean onCheckIsTextEditor(){return true;}
  @Override public InputConnection onCreateInputConnection(EditorInfo info){info.inputType=InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;info.imeOptions=EditorInfo.IME_ACTION_DONE|EditorInfo.IME_FLAG_NO_EXTRACT_UI;return new BaseInputConnection(this,false){@Override public boolean commitText(CharSequence text,int p){input(3,0,0,text.toString());return true;}@Override public boolean deleteSurroundingText(int before,int after){if(before<0||after<0||before>256||after>256)return false;for(int i=0;i<before;i++)input(4,0x0e,0,null);for(int i=0;i<after;i++)input(4,0xd3,0,null);return true;}@Override public boolean performEditorAction(int action){input(4,0x1c,0,null);return true;}@Override public boolean sendKeyEvent(KeyEvent e){if(e.getAction()!=KeyEvent.ACTION_DOWN)return true;if(e.getKeyCode()==KeyEvent.KEYCODE_DEL)input(4,0x0e,0,null);else if(e.getKeyCode()==KeyEvent.KEYCODE_ENTER)input(4,0x1c,0,null);else{int c=e.getUnicodeChar();if(c>=32&&c<=126)input(3,0,0,String.valueOf((char)c));}return true;}};}
 }
 private void showOptions(){} // Navigation deferred in the proof of concept.
 @Override protected void onActivityResult(int request,int result,Intent data){
  super.onActivityResult(request,result,data);if((request!=PICK_FOLDER&&request!=PICK_ZIP)||result!=RESULT_OK||data==null||importing)return;
  final List<Uri> uris=new ArrayList<>();if(data.getClipData()!=null){for(int i=0;i<data.getClipData().getItemCount();i++)uris.add(data.getClipData().getItemAt(i).getUri());}else if(data.getData()!=null)uris.add(data.getData());if(uris.isEmpty())return;
  importing=true;cancelled=false;setup.setVisibility(View.VISIBLE);choose.setEnabled(false);chooseZip.setEnabled(false);cancelImport.setVisibility(View.VISIBLE);status.setText("Checking game data…");
  new Thread(()->{
   File cache=new File(getCacheDir(),"zip-"+UUID.randomUUID());
   try{
    DataFolderImporter.Result imported;
    if(request==PICK_FOLDER){DocumentTreeSource source=new DocumentTreeSource(getContentResolver(),uris.get(0));imported=new DataFolderImporter(source,()->cancelled||destroyed,this::message).run(source.root(),new File(getFilesDir(),"installations"));}
    else {
     if(uris.size()>16)throw new IOException("Select at most 16 ZIP files");if(!cache.mkdir())throw new IOException("Cannot create ZIP cache");List<File> files=new ArrayList<>();long total=0,reported=0;
     for(Uri uri:uris){File file=new File(cache,files.size()+".zip");message("Reading ZIP "+(files.size()+1)+" of "+uris.size()+"…");
      try(InputStream in=getContentResolver().openInputStream(uri);FileOutputStream out=new FileOutputStream(file)){
       if(in==null)throw new IOException("Cannot open selected ZIP");byte[] buffer=new byte[262144];int n;
       while((n=in.read(buffer))!=-1){if(cancelled||destroyed)throw new IOException("Import cancelled");total+=n;if(total>DataFolderImporter.MAX_TOTAL)throw new IOException("ZIP input exceeds 2 GiB");out.write(buffer,0,n);if(total-reported>=32L*1024*1024){reported=total;message("Reading ZIP data: "+total/1024/1024+" MiB");}}
      }files.add(file);
     }
     try(ZipGameSource source=new ZipGameSource(files)){imported=new DataFolderImporter(source,()->cancelled||destroyed,this::message).run(source.root(),new File(getFilesDir(),"installations"));}
    }
    if(!new File(imported.directory,"worlds/sprawl.zwp").isFile()){DataFolderImporter.remove(imported.directory);throw new IOException("Missing worlds/sprawl.zwp, required for the live 3D menu");}
    runOnUiThread(()->finishImport(imported));
   }catch(Exception e){runOnUiThread(()->{if(destroyed)return;importing=false;choose.setEnabled(true);chooseZip.setEnabled(true);cancelImport.setVisibility(View.GONE);message("Import failed: "+e.getMessage()+"\nYour previous game data is unchanged.");});}
   finally{DataFolderImporter.remove(cache);}
  },"ReignDataImport").start();
 }
 private void finishImport(DataFolderImporter.Result imported){
  if(destroyed||cancelled){new Thread(()->DataFolderImporter.remove(imported.directory),"ReignCleanup").start();importing=false;if(!destroyed){choose.setEnabled(true);chooseZip.setEnabled(true);cancelImport.setVisibility(View.GONE);message("Import cancelled. Previous data kept.");}return;}
  final String oldPack=getPreferences(0).getString("pack",null),oldShell=getPreferences(0).getString("shell",null),oldMusic=getPreferences(0).getString("music",null);
  String path=imported.directory.getAbsolutePath(),base=new File(imported.directory,"base.zwp").getAbsolutePath(),shell=new File(imported.directory,"shell.zwp").getAbsolutePath();
  if(!getPreferences(0).edit().putString("dataRoot",path).putString("pack",base).putString("shell",shell).remove("music").commit()){
   getPreferences(0).edit().putString("dataRoot",dataRoot).putString("pack",oldPack).putString("shell",oldShell).putString("music",oldMusic).commit();
   new Thread(()->DataFolderImporter.remove(imported.directory),"ReignCleanup").start();importing=false;choose.setEnabled(true);chooseZip.setEnabled(true);cancelImport.setVisibility(View.GONE);message("Could not save setup. Please try again.");return;
  }
  final String previous=dataRoot;dataRoot=path;importing=false;choose.setEnabled(true);chooseZip.setEnabled(true);cancelImport.setVisibility(View.GONE);setup.setVisibility(View.GONE);
  view.queueEvent(()->{if(view.handle!=0){closeNative(view.handle);view.handle=0;}launchCheckpoint=null;audioGeneration++;view.pack=base;view.shell=shell;view.failed=false;
   if(previous!=null&&!previous.equals(path)){File old=new File(previous);if(old.getName().startsWith("data-")&&old.getParentFile().equals(new File(getFilesDir(),"installations")))new Thread(()->DataFolderImporter.remove(old),"ReignCleanup").start();}
  });view.requestRender();startAudio();
  record("Native shell resource packs imported: "+imported.scenePacks+"/3. Live scene rendering enabled.");
  message(imported.tracks==0?"Game data ready. No music tracks found; menu sound effects remain available.":"Game data ready. "+imported.tracks+" music tracks imported.");
 }
}
