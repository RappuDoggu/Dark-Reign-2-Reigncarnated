package org.reigncarnated.menu;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.opengl.GLSurfaceView;
import android.view.MotionEvent;
import android.view.KeyEvent;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.text.InputType;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import java.io.File;
import java.io.InputStream;
import java.io.FileOutputStream;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
public final class MenuActivity extends Activity {
 static {System.loadLibrary("reign_menu");}
 private native long openNative(String pack,String profiles);
 private native String errorNative();
 private native String drawNative(long h,int w,int height,long ms);
 private native String inputNative(long h,int kind,int x,int y,String text);
 private native String resetGpuNative(long h);
 private native void closeNative(long h);
 private native String[] audioNative(long h);
 private volatile MenuAudioPlayer audioPlayer;
 private MenuView view;private TextView status;private Button choose;private String report="Choose your own base.zwp. Original Login only; shell/gameplay is not implemented; menu sound effects enabled.";
 private void message(String s){if(s==null)return;runOnUiThread(()->{if(!isDestroyed()){report=s;status.setText(s);}});}
 @Override public void onCreate(Bundle saved){super.onCreate(saved);LinearLayout layout=new LinearLayout(this);layout.setOrientation(LinearLayout.VERTICAL);status=new TextView(this);status.setText(report);layout.addView(status);LinearLayout actions=new LinearLayout(this);choose=new Button(this);choose.setText("Choose base.zwp");choose.setOnClickListener(v->{Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT);i.setType("*/*");i.addCategory(Intent.CATEGORY_OPENABLE);startActivityForResult(i,10);});actions.addView(choose);Button keyboard=new Button(this);keyboard.setText("Keyboard");keyboard.setOnClickListener(v->{view.requestFocus();((InputMethodManager)getSystemService(INPUT_METHOD_SERVICE)).showSoftInput(view,InputMethodManager.SHOW_IMPLICIT);});actions.addView(keyboard);Button share=new Button(this);share.setText("Share report");share.setOnClickListener(v->{Intent i=new Intent(Intent.ACTION_SEND);i.setType("text/plain");i.putExtra(Intent.EXTRA_TEXT,report);startActivity(Intent.createChooser(i,"Share test report"));});actions.addView(share);layout.addView(actions);view=new MenuView();layout.addView(view,new LinearLayout.LayoutParams(-1,0,1));setContentView(layout);}
 @Override protected void onResume(){super.onResume();audioPlayer=new MenuAudioPlayer(this,this::message);view.onResume();}
 @Override protected void onPause(){MenuAudioPlayer old=audioPlayer;audioPlayer=null;if(old!=null)old.close();view.queueEvent(()->{if(view.handle!=0){closeNative(view.handle);view.handle=0;}});view.onPause();super.onPause();}
 private final class MenuView extends GLSurfaceView implements GLSurfaceView.Renderer {
  long handle;int width,height;String pack;boolean failed;String gpu="";
  MenuView(){super(MenuActivity.this);pack=getPreferences(0).getString("pack",null);setFocusable(true);setFocusableInTouchMode(true);setEGLContextClientVersion(3);setEGLConfigChooser(8,8,8,8,16,0);setRenderer(this);setRenderMode(RENDERMODE_WHEN_DIRTY);}
  public void onSurfaceCreated(GL10 gl,EGLConfig c){gpu="Device: "+android.os.Build.MANUFACTURER+" "+android.os.Build.MODEL+" API "+android.os.Build.VERSION.SDK_INT+"\nGPU: "+gl.glGetString(GL10.GL_RENDERER);failed=false;if(handle!=0){String error=resetGpuNative(handle);if(error!=null){failed=true;message(error);}}}
  public void onSurfaceChanged(GL10 gl,int w,int h){width=w;height=h;requestRender();}
  public void onDrawFrame(GL10 gl){if(width<=0||height<=0||pack==null||failed)return;if(handle==0){handle=openNative(pack,new File(getFilesDir(),"profiles").getAbsolutePath());if(handle==0){failed=true;message("Native startup failed: "+errorNative());return;}getPreferences(0).edit().putString("pack",pack).apply();message(gpu+"\nOriginal native Login loaded. Touch and ASCII profile text enabled. Menu sound effects enabled; music unavailable. Main-menu shell/gameplay not yet available.");}String e=drawNative(handle,width,height,android.os.SystemClock.uptimeMillis());String[] sounds=audioNative(handle);MenuAudioPlayer player=audioPlayer;if(player!=null&&sounds!=null&&sounds.length>0)runOnUiThread(()->{if(audioPlayer==player)player.accept(sounds);});if(e!=null){failed=true;message("Render failure: "+e);}}
  void input(int kind,int x,int y,String text){queueEvent(()->{if(handle!=0)message(inputNative(handle,kind,x,y,text));});requestRender();}
  @Override public boolean onTouchEvent(MotionEvent e){requestFocus();if(e.getPointerCount()!=1){input(2,-32768,-32768,null);return true;}float scale=Math.min(width/640f,height/480f);if(scale<=0)return true;int x=(int)((e.getX()-(width-640*scale)/2)/scale),y=(int)((e.getY()-(height-480*scale)/2)/scale);int a=e.getActionMasked();if(a==MotionEvent.ACTION_DOWN)input(1,x,y,null);else if(a==MotionEvent.ACTION_UP)input(2,x,y,null);else if(a==MotionEvent.ACTION_CANCEL)input(2,-32768,-32768,null);else if(a==MotionEvent.ACTION_MOVE)input(0,x,y,null);return true;}
  @Override public boolean onCheckIsTextEditor(){return true;}
  @Override public InputConnection onCreateInputConnection(EditorInfo info){info.inputType=InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;info.imeOptions=EditorInfo.IME_ACTION_DONE|EditorInfo.IME_FLAG_NO_EXTRACT_UI;return new BaseInputConnection(this,false){@Override public boolean commitText(CharSequence text,int p){input(3,0,0,text.toString());return true;}@Override public boolean deleteSurroundingText(int before,int after){if(before<0||after<0||before>256||after>256)return false;for(int i=0;i<before;i++)input(4,0x0e,0,null);for(int i=0;i<after;i++)input(4,0xd3,0,null);return true;}@Override public boolean performEditorAction(int action){input(4,0x1c,0,null);return true;}@Override public boolean sendKeyEvent(KeyEvent e){if(e.getAction()!=KeyEvent.ACTION_DOWN)return true;if(e.getKeyCode()==KeyEvent.KEYCODE_DEL)input(4,0x0e,0,null);else if(e.getKeyCode()==KeyEvent.KEYCODE_ENTER)input(4,0x1c,0,null);else{int c=e.getUnicodeChar();if(c>=32&&c<=126)input(3,0,0,String.valueOf((char)c));}return true;}};}
 }
 @Override protected void onActivityResult(int request,int result,Intent data){super.onActivityResult(request,result,data);if(request!=10||result!=RESULT_OK||data==null||data.getData()==null)return;choose.setEnabled(false);message("Copying your selected archive…");new Thread(()->{File file=null;try{file=File.createTempFile("base-",".zwp",getFilesDir());try(InputStream in=getContentResolver().openInputStream(data.getData());FileOutputStream out=new FileOutputStream(file)){if(in==null)throw new java.io.IOException("Cannot open selected file");byte[] buffer=new byte[262144];long total=0;int n;while((n=in.read(buffer))!=-1){total+=n;if(total>512L*1024*1024)throw new java.io.IOException("Archive exceeds 512 MiB limit");out.write(buffer,0,n);}}if(isDestroyed()){file.delete();return;}final String path=file.getAbsolutePath();view.queueEvent(()->{if(view.handle!=0){closeNative(view.handle);view.handle=0;}view.pack=path;view.failed=false;});view.requestRender();}catch(Exception e){if(file!=null)file.delete();message("Archive copy failed: "+e.getMessage());}finally{runOnUiThread(()->{if(!isDestroyed())choose.setEnabled(true);});}},"ReignImport").start();}
}
