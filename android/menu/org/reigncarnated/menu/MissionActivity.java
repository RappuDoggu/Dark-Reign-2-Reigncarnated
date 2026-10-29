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
 private static native long openMission(String root,String writable);
 private static native String missionError();
 private static native String performanceMission(long handle);
 private static native String frameMission(long handle,int width,int height);
 private static native String activeMission(long handle,boolean active);
 private static native void pointerMission(int x,int y,int button,boolean down);
 private static native void keyMission(int key,boolean down);
 private static native void closeMission(long handle);
 private MissionView view;private RenderMetrics metrics;
 private TextView status;
 private volatile boolean resumed;
 @Override public void onCreate(Bundle state){super.onCreate(state);getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);getWindow().getDecorView().setSystemUiVisibility(5894);
  FrameLayout root=new FrameLayout(this);view=new MissionView();root.addView(view);status=new TextView(this);status.setTextColor(0xffffffff);status.setBackgroundColor(0xa0000000);status.setText("Loading original 2crazed mission…");root.addView(status,new FrameLayout.LayoutParams(-1,-2,Gravity.TOP));
  LinearLayout keys=new LinearLayout(this);for(int i=0;i<4;i++){final int key=new int[]{0xcb,0xc8,0xd0,0xcd}[i];Button button=new Button(this);button.setText(new String[]{"◀","▲","▼","▶"}[i]);button.setOnTouchListener((v,event)->{int action=event.getActionMasked();if(action==MotionEvent.ACTION_DOWN)view.queueEvent(()->keyMission(key,true));else if(action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_CANCEL)view.queueEvent(()->keyMission(key,false));return true;});keys.addView(button,new LinearLayout.LayoutParams((int)(48*getResources().getDisplayMetrics().density),(int)(48*getResources().getDisplayMetrics().density)));}
  root.addView(keys,new FrameLayout.LayoutParams(-2,-2,Gravity.BOTTOM|Gravity.LEFT));
  Button deselect=new Button(this);deselect.setText("Deselect");deselect.setOnClickListener(v->{final int x=view.lastX,y=view.lastY;view.queueEvent(()->{pointerMission(x,y,1,true);pointerMission(x,y,1,false);});});root.addView(deselect,new FrameLayout.LayoutParams(-2,-2,Gravity.TOP|Gravity.LEFT));metrics=new RenderMetrics(this,root,"Mission");setContentView(root);
 }
 private void report(String text){runOnUiThread(()->{status.setText(text);status.setVisibility(View.VISIBLE);});}
 @Override protected void onResume(){super.onResume();resumed=true;if(view!=null){view.onResume();view.queueEvent(()->{if(view.handle!=0){String error=activeMission(view.handle,true);if(error!=null)report(error);}});}}
 @Override protected void onPause(){resumed=false;if(view!=null){view.queueEvent(()->{if(view.handle!=0){if(isFinishing()){closeMission(view.handle);view.handle=0;}else {String error=activeMission(view.handle,false);if(error!=null)report(error);}}});view.onPause();}super.onPause();}
 @Override protected void onDestroy(){if(view!=null)view.queueEvent(()->{if(view.handle!=0){closeMission(view.handle);view.handle=0;}});super.onDestroy();}
 private final class MissionView extends GLSurfaceView implements GLSurfaceView.Renderer {
  long handle;int width,height,pressed=-1;boolean failed,ready;volatile int lastX=320,lastY=240;
  MissionView(){super(MissionActivity.this);setEGLContextClientVersion(3);setPreserveEGLContextOnPause(true);setEGLConfigChooser(8,8,8,8,16,0);setRenderer(this);}
  public void onSurfaceCreated(GL10 gl,EGLConfig config){if(handle!=0){closeMission(handle);handle=0;report("Graphics context restored; restarting test mission…");}failed=ready=false;}
  public void onSurfaceChanged(GL10 gl,int w,int h){width=w;height=h;}
  public void onDrawFrame(GL10 gl){if(!resumed||failed||width<=0||height<=0)return;
   if(handle==0){handle=openMission(getIntent().getStringExtra("installation"),new File(getFilesDir(),"mission-runtime").getAbsolutePath());if(handle==0){failed=true;report(missionError());return;}}
   long drawStart=System.nanoTime();String error=frameMission(handle,width,height);metrics.frame(drawStart,System.nanoTime(),()->performanceMission(handle));if(error!=null){failed=true;report(error);}else if(!ready){ready=true;runOnUiThread(()->status.setVisibility(View.GONE));}
  }
  @Override public boolean onTouchEvent(MotionEvent event){float scale=Math.min(width/640f,height/480f);if(scale<=0)return true;final int x=(int)((event.getX()-(width-640*scale)/2)/scale),y=(int)((event.getY()-(height-480*scale)/2)/scale);int action=event.getActionMasked();lastX=x;lastY=y;
   if(action==MotionEvent.ACTION_DOWN){pressed=0;queueEvent(()->pointerMission(x,y,0,true));}
   else if(action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_CANCEL){final int released=pressed;pressed=-1;queueEvent(()->pointerMission(x,y,released,false));}
   else if(action==MotionEvent.ACTION_MOVE)queueEvent(()->pointerMission(x,y,-1,false));return true;
  }
 }
}
