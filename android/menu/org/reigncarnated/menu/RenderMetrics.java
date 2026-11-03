package org.reigncarnated.menu;
import android.app.Activity;
import android.content.Intent;
import android.view.Gravity;
import android.widget.FrameLayout;
import android.widget.TextView;
import java.util.Locale;
import java.util.function.Supplier;

/** Render-thread samples; UI work and JNI diagnostics are limited to once/second. */
final class RenderMetrics {
 private final Activity activity;private final TextView label;private final String stage;
 private final FrameTiming timing=new FrameTiming();private long lastJournal;private volatile String latest="Waiting for frame sample…";
 RenderMetrics(Activity owner,FrameLayout root,String name){activity=owner;stage=name;label=new TextView(owner);label.setTextColor(0xffffffff);label.setBackgroundColor(0xb0000000);label.setTextSize(11);label.setPadding(8,4,8,4);label.setText(name+" performance — tap to share");root.addView(label,new FrameLayout.LayoutParams(-2,-2,Gravity.BOTTOM|Gravity.RIGHT));
  label.setOnClickListener(v->Diagnostics.share(owner,latest,null));
 }
 String latest(){return latest;}
 void reset(){timing.reset();}
 void frame(long start,long end,Supplier<String> details){
  if(!timing.frame(start,end))return;
  long elapsed=end-timing.since,work=timing.work,max=timing.max;int frames=timing.frames;
  String extra=details.get();latest=String.format(Locale.US,"%s %.1f FPS | Native %.1f ms (max %.1f)\n%s",stage,frames*1e9/elapsed,work/1e6/frames,max/1e6,extra==null?"Tap to share performance":extra);
  android.util.Log.i("ReignPerf",latest);if(end-lastJournal>=5_000_000_000L){lastJournal=end;Diagnostics.event(latest);}final String text=latest;activity.runOnUiThread(()->label.setText(text));timing.reset();timing.since=end;
 }
}
