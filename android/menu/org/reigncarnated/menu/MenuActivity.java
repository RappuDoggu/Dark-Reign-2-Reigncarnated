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
import java.io.File;
import java.util.*;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public final class MenuActivity extends Activity {
 static {System.loadLibrary("reign_menu");}
 private static final int PICK_FOLDER=20;
 private native long openNative(String pack,String profiles,String shell);
 private native String errorNative();
 private native String drawNative(long h,int w,int height,long ms);
 private native String inputNative(long h,int kind,int x,int y,String text);
 private native String resetGpuNative(long h);
 private native void closeNative(long h);
 private native String[] audioNative(long h);
 private native float volumeNative(long h);
 private volatile MenuAudioPlayer audioPlayer;
 private MenuView view;private LinearLayout setup;private TextView status;private Button choose,cancelImport;
 private String report="",dataRoot;private volatile boolean importing,cancelled,destroyed;
 private boolean resumed;
 private final StringBuilder history=new StringBuilder();
 private int dp(int n){return Math.round(n*getResources().getDisplayMetrics().density);}
 private boolean hasData(){String base=getPreferences(0).getString("pack",null),shell=getPreferences(0).getString("shell",null);return base!=null&&shell!=null&&new File(base).isFile()&&new File(shell).isFile();}
 private void record(String s){runOnUiThread(()->{if(destroyed)return;report=s;if(history.length()>12000)history.delete(0,6000);history.append(s).append('\n');});}
 private void message(String s){if(s==null)return;runOnUiThread(()->{
  if(destroyed)return;if(s.equals("@quit")){finish();return;}if(s.equals("@options")){showOptions();return;}
  record(s);if(setup.getVisibility()==View.VISIBLE)status.setText(s);else Toast.makeText(this,s,Toast.LENGTH_LONG).show();
 });}
 private void startupFailure(String s){runOnUiThread(()->{if(destroyed)return;setup.setVisibility(View.VISIBLE);message(s);});}
 @Override public void onCreate(Bundle saved){
  super.onCreate(saved);dataRoot=getPreferences(0).getString("dataRoot",null);
  FrameLayout screen=new FrameLayout(this);view=new MenuView();screen.addView(view,new FrameLayout.LayoutParams(-1,-1));
  LinearLayout toolbar=new LinearLayout(this);Button keyboard=new Button(this);keyboard.setText("Keyboard");keyboard.setOnClickListener(v->showKeyboard());toolbar.addView(keyboard);
  Button settings=new Button(this);settings.setText("Settings");settings.setOnClickListener(v->showOptions());toolbar.addView(settings);
  FrameLayout.LayoutParams tools=new FrameLayout.LayoutParams(-2,-2,Gravity.TOP|Gravity.END);screen.addView(toolbar,tools);
  setup=new LinearLayout(this);setup.setOrientation(LinearLayout.VERTICAL);setup.setGravity(Gravity.CENTER);setup.setPadding(dp(28),dp(24),dp(28),dp(24));setup.setBackgroundColor(0xff101710);
  TextView title=new TextView(this);title.setText("REIGNCARNATED");title.setTextColor(0xffaadd99);title.setTextSize(28);title.setGravity(Gravity.CENTER);setup.addView(title);
  TextView description=new TextView(this);description.setText("Select your legally owned Dark Reign 2 installation folder.\n\nIt should contain packs and missions. Music is detected automatically from the music folder. Menu data is copied once into app storage.");description.setTextColor(0xffeeeeee);description.setGravity(Gravity.CENTER);description.setPadding(0,dp(20),0,dp(20));setup.addView(description);
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
 private void pickFolder(){if(importing)return;Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);startActivityForResult(i,PICK_FOLDER);}
 private List<String> musicTracks(){if(dataRoot!=null)return DataFolderImporter.playlist(new File(dataRoot));String legacy=getPreferences(0).getString("music",null);return legacy!=null&&new File(legacy).isFile()?Collections.singletonList(legacy):Collections.emptyList();}
 private void startAudio(){if(!resumed)return;if(audioPlayer!=null)audioPlayer.close();audioPlayer=new MenuAudioPlayer(this,this::message);audioPlayer.setMusicVolume(getPreferences(0).getInt("musicVolume",40)/100f);audioPlayer.playPlaylist(musicTracks());}
 @Override protected void onResume(){super.onResume();resumed=true;startAudio();view.onResume();view.requestRender();}
 @Override protected void onPause(){resumed=false;MenuAudioPlayer old=audioPlayer;audioPlayer=null;if(old!=null)old.close();view.queueEvent(()->{if(view.handle!=0){closeNative(view.handle);view.handle=0;}});view.onPause();super.onPause();}
 @Override protected void onDestroy(){destroyed=true;cancelled=true;super.onDestroy();}
 private final class MenuView extends GLSurfaceView implements GLSurfaceView.Renderer {
  long handle;int width,height;String pack,shell;boolean failed;String gpu="";
  MenuView(){super(MenuActivity.this);pack=getPreferences(0).getString("pack",null);shell=getPreferences(0).getString("shell",null);setFocusable(true);setFocusableInTouchMode(true);setEGLContextClientVersion(3);setEGLConfigChooser(8,8,8,8,16,0);setRenderer(this);setRenderMode(RENDERMODE_WHEN_DIRTY);}
  public void onSurfaceCreated(GL10 gl,EGLConfig c){gpu="Device: "+android.os.Build.MANUFACTURER+" "+android.os.Build.MODEL+" API "+android.os.Build.VERSION.SDK_INT+"\nGPU: "+gl.glGetString(GL10.GL_RENDERER);failed=false;if(handle!=0){String error=resetGpuNative(handle);if(error!=null){failed=true;message(error);}}}
  public void onSurfaceChanged(GL10 gl,int w,int h){width=w;height=h;requestRender();}
  public void onDrawFrame(GL10 gl){if(width<=0||height<=0||pack==null||failed)return;if(handle==0){handle=openNative(pack,new File(getFilesDir(),"profiles").getAbsolutePath(),shell);if(handle==0){failed=true;startupFailure("Native startup failed: "+errorNative());return;}record(gpu+"\nLogin ready. Static main menu; mission gameplay is not yet available.");}String e=drawNative(handle,width,height,android.os.SystemClock.uptimeMillis());String[] sounds=audioNative(handle);MenuAudioPlayer player=audioPlayer;if(player!=null&&sounds!=null&&sounds.length>0)runOnUiThread(()->{if(audioPlayer==player)player.accept(sounds);});if(e!=null){failed=true;message("Render failure: "+e);}}
  void input(int kind,int x,int y,String text){queueEvent(()->{if(handle!=0)message(inputNative(handle,kind,x,y,text));});requestRender();}
  @Override public boolean onTouchEvent(MotionEvent e){requestFocus();if(e.getPointerCount()!=1){input(2,-32768,-32768,null);return true;}float scale=Math.min(width/640f,height/480f);if(scale<=0)return true;int x=(int)((e.getX()-(width-640*scale)/2)/scale),y=(int)((e.getY()-(height-480*scale)/2)/scale);int a=e.getActionMasked();if(a==MotionEvent.ACTION_DOWN)input(1,x,y,null);else if(a==MotionEvent.ACTION_UP)input(2,x,y,null);else if(a==MotionEvent.ACTION_CANCEL)input(2,-32768,-32768,null);else if(a==MotionEvent.ACTION_MOVE)input(0,x,y,null);return true;}
  @Override public boolean onCheckIsTextEditor(){return true;}
  @Override public InputConnection onCreateInputConnection(EditorInfo info){info.inputType=InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;info.imeOptions=EditorInfo.IME_ACTION_DONE|EditorInfo.IME_FLAG_NO_EXTRACT_UI;return new BaseInputConnection(this,false){@Override public boolean commitText(CharSequence text,int p){input(3,0,0,text.toString());return true;}@Override public boolean deleteSurroundingText(int before,int after){if(before<0||after<0||before>256||after>256)return false;for(int i=0;i<before;i++)input(4,0x0e,0,null);for(int i=0;i<after;i++)input(4,0xd3,0,null);return true;}@Override public boolean performEditorAction(int action){input(4,0x1c,0,null);return true;}@Override public boolean sendKeyEvent(KeyEvent e){if(e.getAction()!=KeyEvent.ACTION_DOWN)return true;if(e.getKeyCode()==KeyEvent.KEYCODE_DEL)input(4,0x0e,0,null);else if(e.getKeyCode()==KeyEvent.KEYCODE_ENTER)input(4,0x1c,0,null);else{int c=e.getUnicodeChar();if(c>=32&&c<=126)input(3,0,0,String.valueOf((char)c));}return true;}};}
 }
 private void showOptions(){
  if(importing){setup.setVisibility(View.VISIBLE);return;}
  view.queueEvent(()->{final int volume=Math.round(volumeNative(view.handle)*100);runOnUiThread(()->{
   if(destroyed||isFinishing())return;
   LinearLayout panel=new LinearLayout(this);panel.setOrientation(LinearLayout.VERTICAL);panel.setPadding(dp(20),dp(8),dp(20),dp(8));
   TextView label=new TextView(this);label.setText("Menu effects: "+volume+"%");panel.addView(label);
   SeekBar effects=new SeekBar(this);effects.setMax(100);effects.setProgress(volume);panel.addView(effects);
   effects.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onProgressChanged(SeekBar b,int value,boolean user){label.setText("Menu effects: "+value+"%");}public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){view.input(5,b.getProgress(),0,null);}});
   TextView musicLabel=new TextView(this);musicLabel.setText("Music volume • "+musicTracks().size()+" tracks detected");panel.addView(musicLabel);
   SeekBar music=new SeekBar(this);music.setMax(100);music.setProgress(getPreferences(0).getInt("musicVolume",40));panel.addView(music);
   music.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onProgressChanged(SeekBar b,int value,boolean user){if(user){getPreferences(0).edit().putInt("musicVolume",value).apply();if(audioPlayer!=null)audioPlayer.setMusicVolume(value/100f);}}public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){}});
   Button folder=new Button(this);folder.setText("Change game data folder");panel.addView(folder);
   Button share=new Button(this);share.setText("Share diagnostic report");share.setOnClickListener(v->{Intent i=new Intent(Intent.ACTION_SEND);i.setType("text/plain");i.putExtra(Intent.EXTRA_TEXT,"Reigncarnated 0.3\n"+history.toString());startActivity(Intent.createChooser(i,"Share test report"));});panel.addView(share);
   TextView note=new TextView(this);note.setText("Original main menu with a static backdrop. Missions and the moving 3D shell are still in development.");panel.addView(note);
   ScrollView scroll=new ScrollView(this);scroll.addView(panel);
   android.app.AlertDialog dialog=new android.app.AlertDialog.Builder(this).setTitle("Settings").setView(scroll).setPositiveButton("Done",(d,w)->view.input(5,effects.getProgress(),0,null)).create();
   folder.setOnClickListener(v->{dialog.dismiss();pickFolder();});dialog.show();
  });});
 }
 @Override protected void onActivityResult(int request,int result,Intent data){
  super.onActivityResult(request,result,data);if(request!=PICK_FOLDER||result!=RESULT_OK||data==null||data.getData()==null||importing)return;
  importing=true;cancelled=false;setup.setVisibility(View.VISIBLE);choose.setEnabled(false);cancelImport.setVisibility(View.VISIBLE);status.setText("Checking installation…");
  final DocumentTreeSource source=new DocumentTreeSource(getContentResolver(),data.getData());
  new Thread(()->{
   try{
    DataFolderImporter.Result imported=new DataFolderImporter(source,()->cancelled||destroyed,this::message).run(source.root(),new File(getFilesDir(),"installations"));
    runOnUiThread(()->finishImport(imported));
   }catch(Exception e){runOnUiThread(()->{if(destroyed)return;importing=false;choose.setEnabled(true);cancelImport.setVisibility(View.GONE);message("Import failed: "+e.getMessage()+"\nYour previous game data is unchanged.");});}
  },"ReignFolderImport").start();
 }
 private void finishImport(DataFolderImporter.Result imported){
  if(destroyed||cancelled){new Thread(()->DataFolderImporter.remove(imported.directory),"ReignCleanup").start();importing=false;if(!destroyed){choose.setEnabled(true);cancelImport.setVisibility(View.GONE);message("Import cancelled. Previous data kept.");}return;}
  final String oldPack=getPreferences(0).getString("pack",null),oldShell=getPreferences(0).getString("shell",null),oldMusic=getPreferences(0).getString("music",null);
  String path=imported.directory.getAbsolutePath(),base=new File(imported.directory,"base.zwp").getAbsolutePath(),shell=new File(imported.directory,"shell.zwp").getAbsolutePath();
  if(!getPreferences(0).edit().putString("dataRoot",path).putString("pack",base).putString("shell",shell).remove("music").commit()){
   getPreferences(0).edit().putString("dataRoot",dataRoot).putString("pack",oldPack).putString("shell",oldShell).putString("music",oldMusic).commit();
   new Thread(()->DataFolderImporter.remove(imported.directory),"ReignCleanup").start();importing=false;choose.setEnabled(true);cancelImport.setVisibility(View.GONE);message("Could not save setup. Please try again.");return;
  }
  final String previous=dataRoot;dataRoot=path;importing=false;choose.setEnabled(true);cancelImport.setVisibility(View.GONE);setup.setVisibility(View.GONE);
  view.queueEvent(()->{if(view.handle!=0){closeNative(view.handle);view.handle=0;}view.pack=base;view.shell=shell;view.failed=false;
   if(previous!=null&&!previous.equals(path)){File old=new File(previous);if(old.getName().startsWith("data-")&&old.getParentFile().equals(new File(getFilesDir(),"installations")))new Thread(()->DataFolderImporter.remove(old),"ReignCleanup").start();}
  });view.requestRender();startAudio();
  message(imported.tracks==0?"Game data ready. No music tracks found; menu sound effects remain available.":"Game data ready. "+imported.tracks+" music tracks imported.");
 }
}
