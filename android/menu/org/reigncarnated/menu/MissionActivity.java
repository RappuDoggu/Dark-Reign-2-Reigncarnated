package org.reigncarnated.menu;
import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.*;
import android.widget.*;
import java.io.File;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/** Device-validation entry point for the original ARM64 mission engine. */
public final class MissionActivity extends Activity {
 static {System.loadLibrary("reign_mission");}
 private static native long openMission(String root,String writable,String group,String mission,boolean campaign,int opponents,int seed,String difficulty);
 private static native String controlMission(long h,int action,float x,float y);
 private static native boolean placingMission(long h);
 private static native void abandonMission(long h);
 private static native String missionError();
 private static native String loadingMission();
 private static native String effectsMission(long handle,float volume);
 private static native String debugMission(long h,int action);
 private static native String diagnosticsMission(long h);
 private static native int[] captureMission(long h);
 private MenuAudioPlayer music;

 private static native String performanceMission(long handle);
 private static native String frameMission(long handle,int width,int height);
 private static native String activeMission(long handle,boolean active);
 private static native void pointerMission(int x,int y,int button,boolean down);
 private static native void keyMission(int key,boolean down);
 private static native void closeMission(long handle);
 private MissionView view;private RenderMetrics metrics;
 private TextView status;
 private volatile boolean resumed;
 private volatile boolean simulationReady;
 private final android.os.Handler loadingHandler=new android.os.Handler(android.os.Looper.getMainLooper());
 private final Runnable loadingPoll=new Runnable(){public void run(){if(!resumed||simulationReady)return;String progress=loadingMission();if(progress.startsWith("FAILED\n")){simulationReady=false;status.setText(progress+"\nTap for diagnostics");status.setVisibility(View.VISIBLE);return;}if(progress.startsWith("READY\n")){simulationReady=true;status.setVisibility(View.GONE);}else{String[] lines=progress.split("\n");status.setText("Loading "+intentValue("mission","")+" • "+(lines.length>1?lines[1]:"starting")+"\n"+(lines.length>2?lines[2]:"")+" • Tap for loading report");status.setVisibility(View.VISIBLE);}loadingHandler.postDelayed(this,500);}};
 private TextView placementHint;
 private int dp(int value){return Math.round(value*getResources().getDisplayMetrics().density);}
 private Button actionButton(String label,String hint,Runnable action){
  Button button=new Button(this);button.setText(label);button.setMinHeight(dp(48));button.setTooltipText(hint);button.setOnClickListener(v->action.run());return button;
 }
 @Override public void onCreate(Bundle state){
  super.onCreate(state);AndroidOptions.importFrom(this,getIntent());Diagnostics.init(this);getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);getWindow().getDecorView().setSystemUiVisibility(5894);
  FrameLayout root=new FrameLayout(this);view=new MissionView();root.addView(view);
  status=new TextView(this);status.setTextColor(0xffffffff);status.setBackgroundColor(0xc0000000);status.setText("Loading "+intentValue("mission","2crazed")+"…");root.addView(status,new FrameLayout.LayoutParams(-1,-2,Gravity.BOTTOM));
  status.setOnClickListener(v->shareLoadingReport());
  LinearLayout cameraTools=new LinearLayout(this);cameraTools.setBackgroundColor(0xb0000000);
  for(int i=0;i<4;i++){
   final int key=new int[]{0xcb,0xc8,0xd0,0xcd}[i];Button arrow=new Button(this);arrow.setText(new String[]{"◀","▲","▼","▶"}[i]);
   arrow.setOnTouchListener((v,event)->{int action=event.getActionMasked();if(action==MotionEvent.ACTION_DOWN)view.queueEvent(()->keyMission(key,true));else if(action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_CANCEL)view.queueEvent(()->keyMission(key,false));return true;});
   cameraTools.addView(arrow,new LinearLayout.LayoutParams(dp(56),dp(48)));
  }
  cameraTools.setVisibility(View.GONE);FrameLayout.LayoutParams cameraPosition=new FrameLayout.LayoutParams(-2,-2,Gravity.TOP|Gravity.CENTER_HORIZONTAL);cameraPosition.topMargin=dp(80);root.addView(cameraTools,cameraPosition);
  LinearLayout toolbar=new LinearLayout(this);toolbar.setBackgroundColor(0xc0182418);
  toolbar.addView(actionButton("Left click","Click at the world cursor; confirms the building preview",()->view.touch.click()));
  toolbar.addView(actionButton("Deselect","Clear selection and cancel building placement",()->{view.cancelTouch();view.control(1,0,0);}));
  HorizontalScrollView scroll=new HorizontalScrollView(this);scroll.setHorizontalScrollBarEnabled(false);LinearLayout more=new LinearLayout(this);
  more.addView(actionButton("Build ↶","Rotate the selected building preview counterclockwise",()->view.control(2,0,0)));
  more.addView(actionButton("Build ↷","Rotate the selected building preview clockwise",()->view.control(3,0,0)));
  more.addView(actionButton("Camera","Two fingers pan the map only. Three-finger tap rotates the building preview.",()->cameraTools.setVisibility(cameraTools.getVisibility()==View.VISIBLE?View.GONE:View.VISIBLE)));
  more.addView(actionButton("Debug","Collect or share diagnostics",this::showDebugTools));scroll.addView(more);toolbar.addView(scroll,new LinearLayout.LayoutParams(0,-2,1));root.addView(toolbar,new FrameLayout.LayoutParams(-1,-2,Gravity.TOP));
  more.addView(actionButton("Options","Audio, display and touch options",()->AndroidOptions.show(this,()->{AndroidOptions.applyWindow(this,metrics);if(music!=null)music.setMusicVolume(AndroidOptions.music(this)/100f);view.queueEvent(()->{if(view.handle!=0){String error=effectsMission(view.handle,AndroidOptions.effects(this));if(error!=null)report(error);}});})));
  placementHint=new TextView(this);placementHint.setText("Tap to left-click • Drag to select • Hold to build/place • Three-finger tap rotates");placementHint.setTextColor(0xffffffff);placementHint.setBackgroundColor(0xc0000000);placementHint.setVisibility(View.GONE);FrameLayout.LayoutParams hintPosition=new FrameLayout.LayoutParams(-1,-2,Gravity.TOP);hintPosition.topMargin=dp(52);root.addView(placementHint,hintPosition);
  metrics=new RenderMetrics(this,root,"Mission");setContentView(root);
 }
 private void showDebugTools(){
  if(!simulationReady){new android.app.AlertDialog.Builder(this).setTitle(view.failed?"Mission stopped":"Mission loading").setMessage(loadingMission()).setPositiveButton("Save / share diagnostics ZIP",(d,w)->{if(view.failed)collectDiagnostics();else shareLoadingReport();}).setNegativeButton("Close",null).show();return;}
  new android.app.AlertDialog.Builder(this).setItems(new String[]{"Share report","GPU checks","Fast stream","Synchronized stream","Audit next frame","Capture frame and report","Toggle sky","Toggle objects","Toggle HUD","Collect diagnostics ZIP (all in one)","Compare distance fog (keeps fog of war)"},(d,which)->{
   if(which==9){collectDiagnostics();return;}if(which==2||which==3)getSharedPreferences("debug",0).edit().putBoolean("synchronized",which==3).apply();
   view.queueEvent(()->{if(view.handle==0)return;if(which==0||which==5)Diagnostics.share(this,metrics.latest()+"\n"+diagnosticsMission(view.handle),which==5?captureMission(view.handle):null);else Diagnostics.event(debugMission(view.handle,which==10?11:which>=6?which+2:which));});
  }).show();
 }
 private void shareLoadingReport(){java.util.Map<String,String> reports=new java.util.LinkedHashMap<>();reports.put("loading.txt","Mission "+intentValue("group","")+"/"+intentValue("mission","")+"\n"+loadingMission());reports.put("performance.txt",metrics.latest());Diagnostics.shareZip(this,reports,new java.util.LinkedHashMap<>());}
 private String intentValue(String key,String fallback){String value=getIntent().getStringExtra(key);return value==null?fallback:value;}
 private void report(String text){runOnUiThread(()->{status.setText(text);status.setVisibility(View.VISIBLE);});}
 @Override protected void onResume(){super.onResume();Diagnostics.event("Mission resume");metrics.reset();AndroidOptions.applyWindow(this,metrics);resumed=true;loadingHandler.removeCallbacks(loadingPoll);loadingHandler.postDelayed(loadingPoll,500);
  music=new MenuAudioPlayer(this,Diagnostics::event);music.setMusicVolume(AndroidOptions.music(this)/100f);String musicRoot=getIntent().getStringExtra("musicRoot");if(musicRoot!=null)music.playPlaylist(DataFolderImporter.playlist(new File(musicRoot)));if(view!=null){view.onResume();view.queueEvent(()->{if(view.handle!=0){String error=activeMission(view.handle,true);if(error!=null)report(error);}});}}
 @Override protected void onPause(){Diagnostics.event("Mission pause");loadingHandler.removeCallbacks(loadingPoll);if(music!=null){music.close();music=null;}resumed=false;if(view!=null){view.cancelTouch();view.queueEvent(()->{view.tapRelease=false;if(view.sweep!=null)view.sweep.cancel();if(view.handle!=0)Diagnostics.saveMissionState(this,intentValue("group","")+"/"+intentValue("mission",""),metrics.latest()+"\n"+diagnosticsMission(view.handle));if(isFinishing()){if(view.handle!=0){closeMission(view.handle);view.handle=0;}}else if(view.handle!=0){String error=activeMission(view.handle,false);if(error!=null)report(error);}});view.onPause();}super.onPause();}
 @Override protected void onDestroy(){if(view!=null)view.queueEvent(()->{if(view.handle!=0){closeMission(view.handle);view.handle=0;}});super.onDestroy();}
 @Override public void onBackPressed(){android.content.Intent result=new android.content.Intent();AndroidOptions.exportTo(this,result);setResult(RESULT_OK,result);super.onBackPressed();}
 @Override protected void onActivityResult(int request,int result,android.content.Intent data){super.onActivityResult(request,result,data);Diagnostics.saveResult(this,request,result,data);}
 private void collectDiagnostics(){
  android.widget.Toast.makeText(this,"Collecting fast/synchronized frames and GPU checks. Keep this screen open until the ZIP share dialog appears.",android.widget.Toast.LENGTH_LONG).show();
  final boolean original=getSharedPreferences("debug",0).getBoolean("synchronized",false);
  view.queueEvent(()->{if(view.sweep!=null&&!view.sweep.done())return;if(view.handle==0){Diagnostics.event("Diagnostic collection unavailable: no native handle");Diagnostics.shareZip(this,java.util.Collections.singletonMap("collection.txt","No native handle\n"+metrics.latest()),new java.util.LinkedHashMap<>());return;}
   view.sweep=new DiagnosticSweep(new DiagnosticSweep.Backend(){public String debug(int action){return debugMission(view.handle,action);}public String report(){return metrics.latest()+"\n"+diagnosticsMission(view.handle);}public int[] capture(){return captureMission(view.handle);}},(reports,pictures)->Diagnostics.shareZip(MissionActivity.this,reports,pictures),original);
   view.sweep.start();if(view.failed)view.sweep.abort("Render loop previously failed; exporting available diagnostics");
  });
 }
 private final class MissionView extends GLSurfaceView implements GLSurfaceView.Renderer {
  DiagnosticSweep sweep;
  final android.os.Handler holdHandler=new android.os.Handler(android.os.Looper.getMainLooper());
  void cancelTouch(){holdHandler.removeCallbacks(holdCheck);touch.cancel();}
  long handle;int width,height;volatile boolean failed;boolean ready,tapRelease,placing;int tapX,tapY;
  final MissionTouch touch=new MissionTouch(new MissionTouch.Sink(){
   public void move(int x,int y){queueEvent(()->pointerMission(x,y,-1,false));}
   public void press(int x,int y){queueEvent(()->{if(tapRelease){pointerMission(tapX,tapY,0,false);tapRelease=false;}pointerMission(x,y,0,true);});}
   public void release(int x,int y){queueEvent(()->pointerMission(x,y,0,false));}
   public void click(int x,int y){queueEvent(()->{if(tapRelease)pointerMission(tapX,tapY,0,false);pointerMission(x,y,0,true);tapX=x;tapY=y;tapRelease=true;});}
   public void camera(int action,float x,float y){if(action==2||action==3)performHapticFeedback(HapticFeedbackConstants.CLOCK_TICK);float speed=AndroidOptions.pan(MissionActivity.this);control(action,x*speed,y*speed);}
  });
  final Runnable holdCheck=()->{if(resumed&&touch.hold(android.os.SystemClock.uptimeMillis()))performHapticFeedback(HapticFeedbackConstants.LONG_PRESS);};
  MissionView(){super(MissionActivity.this);setEGLContextClientVersion(3);setPreserveEGLContextOnPause(true);setEGLConfigChooser(8,8,8,8,16,0);setRenderer(this);}
  public void onSurfaceCreated(GL10 gl,EGLConfig config){if(sweep!=null){sweep.cancel();sweep=null;}Diagnostics.event("Mission GPU: "+gl.glGetString(GL10.GL_RENDERER)+" | "+gl.glGetString(GL10.GL_VERSION));if(handle!=0){abandonMission(handle);closeMission(handle);handle=0;report("Graphics context restored; restarting test mission…");}failed=ready=placing=tapRelease=simulationReady=false;runOnUiThread(()->{cancelTouch();touch.aim(false);placementHint.setVisibility(View.GONE);});}
  public void onSurfaceChanged(GL10 gl,int w,int h){width=w;height=h;}
  public void onDrawFrame(GL10 gl){if(!resumed||failed||width<=0||height<=0)return;
   if(handle==0){handle=openMission(getIntent().getStringExtra("installation"),new File(getFilesDir(),"mission-runtime").getAbsolutePath(),intentValue("group","missions/multiplayer"),intentValue("mission","2crazed"),getIntent().getBooleanExtra("campaign",false),getIntent().getIntExtra("opponents",0),getIntent().getIntExtra("seed",1),intentValue("difficulty",""));if(handle==0){failed=true;report(missionError());return;}effectsMission(handle,AndroidOptions.effects(MissionActivity.this));debugMission(handle,getSharedPreferences("debug",0).getBoolean("synchronized",false)?3:2);}
   long drawStart=System.nanoTime();String error=frameMission(handle,width,height);boolean nextPlacement=placingMission(handle);if(nextPlacement!=placing){placing=nextPlacement;runOnUiThread(()->{touch.aim(nextPlacement);placementHint.setVisibility(nextPlacement?View.VISIBLE:View.GONE);});}if(tapRelease){pointerMission(tapX,tapY,0,false);tapRelease=false;}long drawEnd=System.nanoTime();metrics.frame(drawStart,drawEnd,()->performanceMission(handle));if(sweep!=null){if(error!=null)sweep.abort(error);else sweep.frame(drawStart,drawEnd);}if(error!=null){Diagnostics.saveMissionState(MissionActivity.this,intentValue("group","")+"/"+intentValue("mission",""),metrics.latest()+"\n"+diagnosticsMission(handle));failed=true;simulationReady=false;runOnUiThread(()->cancelTouch());report(error);}else if(!ready&&loadingMission().startsWith("READY\n")){ready=simulationReady=true;runOnUiThread(()->status.setVisibility(View.GONE));}
  }
  void control(int action,float x,float y){queueEvent(()->{if(handle!=0){String error=controlMission(handle,action,x,y);if(error!=null){Diagnostics.event(error);runOnUiThread(()->Toast.makeText(MissionActivity.this,error,Toast.LENGTH_SHORT).show());}}});}
  @Override public boolean onTouchEvent(MotionEvent event){
   float scale=Math.min(width/640f,height/480f);if(scale<=0)return true;
   if(!simulationReady){cancelTouch();return true;}
   int action=event.getActionMasked();float offsetX=(width-640*scale)/2,offsetY=(height-480*scale)/2;
   float x=(event.getX()-offsetX)/scale,y=(event.getY()-offsetY)/scale;
   if(action==MotionEvent.ACTION_CANCEL){cancelTouch();return true;}
   if(action==MotionEvent.ACTION_DOWN){holdHandler.removeCallbacks(holdCheck);touch.down(x,y,event.getEventTime());holdHandler.postDelayed(holdCheck,MissionTouch.HOLD_MS);return true;}
   if(event.getPointerCount()>=2){holdHandler.removeCallbacks(holdCheck);int n=event.getPointerCount();int[] ids=new int[n];float[] xs=new float[n],ys=new float[n];for(int i=0;i<n;i++){ids[i]=event.getPointerId(i);xs[i]=(event.getX(i)-offsetX)/scale;ys[i]=(event.getY(i)-offsetY)/scale;}touch.fingers(ids,xs,ys,event.getEventTime());if(action==MotionEvent.ACTION_POINTER_UP)touch.pairEnd(event.getEventTime());return true;}
   if(action==MotionEvent.ACTION_UP){holdHandler.removeCallbacks(holdCheck);touch.up(x,y,event.getEventTime());}else if(action==MotionEvent.ACTION_MOVE)touch.move(x,y);return true;
  }
 }
}
