package org.reigncarnated.menu;
import java.util.*;
public final class MissionTouchTest {
 static int checks;
 static void check(boolean b){checks++;if(!b)throw new AssertionError("touch check "+checks);}
 static final class Sink implements MissionTouch.Sink {
  List<String> clicks=new ArrayList<>();List<Integer> actions=new ArrayList<>();List<Float> pans=new ArrayList<>();int moves;
  public void move(int x,int y){moves++;}
  public void click(int x,int y){clicks.add(x+","+y);}
  public void camera(int a,float x,float y){check(a==2||a==3||a==5);actions.add(a);if(a==5)pans.add(x);}
 }
 static void tap(MissionTouch t,float x,float y,long time){t.down(x,y,time);t.up(x,y,time+40);}
 static void fingers(MissionTouch t,int count,float x,float spread){int[] ids=new int[count];float[] xs=new float[count],ys=new float[count];for(int i=0;i<count;i++){ids[i]=i;xs[i]=x+(i-1)*spread;ys[i]=220;}t.fingers(ids,xs,ys);}
 public static void main(String[] args){
  Sink s=new Sink();MissionTouch t=new MissionTouch(s);
  tap(t,200,200,0);check(s.clicks.isEmpty());tap(t,200,200,150);check(s.clicks.equals(Arrays.asList("200,200")));
  tap(t,200,200,600);check(s.clicks.size()==1);tap(t,200,200,1100);check(s.clicks.size()==1);tap(t,400,200,1250);check(s.clicks.size()==1);tap(t,400,200,1400);check(s.clicks.size()==2);
  t.down(200,200,2000);t.move(240,240);t.up(240,240,2100);check(s.clicks.size()==2);tap(t,240,240,2200);check(s.clicks.size()==2);
  t.aim(true);tap(t,250,250,3000);tap(t,250,250,3150);check(s.clicks.get(s.clicks.size()-1).equals("250,250"));
  int before=s.clicks.size();t.down(200,200,4000);fingers(t,2,250,40);fingers(t,2,280,40);check(s.actions.equals(Arrays.asList(5)));check(Math.abs(s.pans.get(0)+3.6f)<.01f);t.pairEnd();t.move(220,220);t.up(220,220,4200);check(s.clicks.size()==before);
  s.actions.clear();fingers(t,2,250,40);fingers(t,2,250,90);check(s.actions.equals(Arrays.asList(5))); // asymmetric centroid shifts are pan only
  t.cancel();s.actions.clear();t.fingers(new int[]{1,2},new float[]{200,300},new float[]{200,200});t.fingers(new int[]{2,1},new float[]{340,160},new float[]{200,200});check(s.actions.isEmpty());
  t.cancel();s.actions.clear();fingers(t,3,300,30);fingers(t,3,340,30);check(s.actions.equals(Arrays.asList(3)));fingers(t,3,260,30);check(s.actions.equals(Arrays.asList(3,2,2)));
  t.pairEnd();s.actions.clear();fingers(t,2,300,30);check(s.actions.isEmpty());fingers(t,2,320,30);check(s.actions.equals(Arrays.asList(5)));
  t.aim(false);t.cancel();s.actions.clear();fingers(t,3,300,30);fingers(t,3,400,30);check(s.actions.isEmpty());
  t.cancel();s.actions.clear();fingers(t,4,300,30);fingers(t,4,400,30);check(s.actions.isEmpty());
  tap(t,100,100,5000);t.cancel();tap(t,100,100,5150);check(s.clicks.size()==before);t.up(100,100,5200);check(s.clicks.size()==before);
  tap(t,-5,-5,6000);tap(t,5,5,6150);check(s.clicks.size()==before);t.cancel();t.click();check(s.clicks.size()==before+1);
  t.cancel();s.actions.clear();t.fingers(new int[]{1,2},new float[]{200,300},new float[]{200,200});t.fingers(new int[]{1,3},new float[]{500,600},new float[]{200,200});check(s.actions.isEmpty());
  System.out.println("Double-tap-only clicks, single-finger aiming, two-finger pan without zoom/rotation, three-finger building steps, pointer changes and cancellation PASS ("+checks+" checks)");
 }
}
