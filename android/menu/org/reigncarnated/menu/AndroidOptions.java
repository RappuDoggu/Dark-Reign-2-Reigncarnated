package org.reigncarnated.menu;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.SharedPreferences;
import android.view.WindowManager;
import android.widget.*;

/** Android settings shared by the shell and mission Activities. */
final class AndroidOptions {
 static SharedPreferences preferences(Activity a){return a.getSharedPreferences("android-options",0);}
 static int music(Activity a){String legacy=MenuActivity.class.getName(),prefix=a.getPackageName()+".";if(legacy.startsWith(prefix))legacy=legacy.substring(prefix.length());return preferences(a).getInt("music",a.getSharedPreferences(legacy,0).getInt("musicVolume",40));}
 static float effects(Activity a){return preferences(a).getInt("effects",100)/100f;}
 static float pan(Activity a){return preferences(a).getInt("pan",100)/100f;}
 static void applyWindow(Activity a,RenderMetrics metrics){
  SharedPreferences p=preferences(a);if(p.getBoolean("awake",true))a.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);else a.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
  WindowManager.LayoutParams w=a.getWindow().getAttributes();int brightness=p.getInt("brightness",0);w.screenBrightness=brightness==0?WindowManager.LayoutParams.BRIGHTNESS_OVERRIDE_NONE:Math.max(.01f,brightness/100f);a.getWindow().setAttributes(w);
  metrics.visible(p.getBoolean("performance",true));
 }
 private static SeekBar slider(Activity a,LinearLayout panel,String title,int maximum,int value,String zero){
  TextView label=new TextView(a);panel.addView(label);SeekBar slider=new SeekBar(a);slider.setMax(maximum);slider.setProgress(value);panel.addView(slider);
  slider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onProgressChanged(SeekBar s,int v,boolean user){label.setText(title+": "+(v==0&&zero!=null?zero:v+"%"));}public void onStartTrackingTouch(SeekBar s){}public void onStopTrackingTouch(SeekBar s){}});
  label.setText(title+": "+(value==0&&zero!=null?zero:value+"%"));return slider;
 }
 static void show(Activity a,Runnable applied){
  SharedPreferences p=preferences(a);LinearLayout panel=new LinearLayout(a);panel.setOrientation(LinearLayout.VERTICAL);int pad=Math.round(20*a.getResources().getDisplayMetrics().density);panel.setPadding(pad,pad,pad,pad);
  TextView graphics=new TextView(a);graphics.setText("Graphics: High detail • full meshes • 32-bit textures • trilinear filtering\nTwo fingers pan. Three fingers rotate building previews. Double-tap to click.");panel.addView(graphics);
  SeekBar music=slider(a,panel,"Music volume",100,music(a),"Muted"),effects=slider(a,panel,"Sound effects volume",100,p.getInt("effects",100),"Muted"),pan=slider(a,panel,"Map pan speed",200,p.getInt("pan",100),null),brightness=slider(a,panel,"Screen brightness",100,p.getInt("brightness",0),"Use Android setting");
  pan.setMin(10);
  CheckBox awake=new CheckBox(a);awake.setText("Keep screen awake while playing");awake.setChecked(p.getBoolean("awake",true));panel.addView(awake);
  CheckBox performance=new CheckBox(a);performance.setText("Show FPS and frame timings");performance.setChecked(p.getBoolean("performance",true));panel.addView(performance);
  ScrollView scroll=new ScrollView(a);scroll.addView(panel);
  new AlertDialog.Builder(a).setTitle("Android options").setView(scroll).setPositiveButton("Apply",(d,w)->{p.edit().putInt("music",music.getProgress()).putInt("effects",effects.getProgress()).putInt("pan",Math.max(10,pan.getProgress())).putInt("brightness",brightness.getProgress()).putBoolean("awake",awake.isChecked()).putBoolean("performance",performance.isChecked()).apply();applied.run();}).setNegativeButton("Cancel",null).show();
 }
}
