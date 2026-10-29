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
 private long since;private long work,max;private int frames;private volatile String latest="Waiting for frame sample…";
 RenderMetrics(Activity owner,FrameLayout root,String name){activity=owner;stage=name;label=new TextView(owner);label.setTextColor(0xffffffff);label.setBackgroundColor(0xb0000000);label.setTextSize(11);label.setPadding(8,4,8,4);label.setText(name+" performance — tap to share");root.addView(label,new FrameLayout.LayoutParams(-2,-2,Gravity.BOTTOM|Gravity.RIGHT));
  label.setOnClickListener(v->{Intent i=new Intent(Intent.ACTION_SEND);i.setType("text/plain");i.putExtra(Intent.EXTRA_TEXT,"Reigncarnated 0.7 Performance Test\n"+android.os.Build.MANUFACTURER+" "+android.os.Build.MODEL+" API "+android.os.Build.VERSION.SDK_INT+"\n"+latest);owner.startActivity(Intent.createChooser(i,"Share performance"));});
 }
 String latest(){return latest;}
 void frame(long start,long end,Supplier<String> details){
  if(since==0||end-since>3_000_000_000L){since=end;frames=0;work=max=0;return;}
  long elapsed=end-since;long cost=end-start;work+=cost;max=Math.max(max,cost);frames++;
  if(elapsed<1_000_000_000L)return;
  String extra=details.get();latest=String.format(Locale.US,"%s %.1f FPS | CPU %.1f ms (max %.1f)\n%s",stage,frames*1e9/elapsed,work/1e6/frames,max/1e6,extra==null?"Tap to share performance":extra);
  final String text=latest;activity.runOnUiThread(()->label.setText(text));since=end;frames=0;work=max=0;
 }
}
