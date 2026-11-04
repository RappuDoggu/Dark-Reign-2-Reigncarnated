package org.reigncarnated.menu;
import java.util.*;
public final class MissionTouchTest {
 static int checks;
 static void check(boolean b){checks++;if(!b)throw new AssertionError("touch check "+checks);}
 static final class Sink implements MissionTouch.Sink {
  List<String> clicks=new ArrayList<>(),presses=new ArrayList<>(),releases=new ArrayList<>();List<Integer> actions=new ArrayList<>();List<Float> pans=new ArrayList<>();
  public void move(int x,int y){}
  public void press(int x,int y){presses.add(x+","+y);}
  public void release(int x,int y){releases.add(x+","+y);}
  public void click(int x,int y){clicks.add(x+","+y);}
  public void camera(int a,float x,float y){check(a==3||a==5);actions.add(a);if(a==5)pans.add(x);}
 }
 static void tap(MissionTouch t,float x,float y,long time){t.down(x,y,time);t.up(x,y,time+40);}
 static void fingers(MissionTouch t,int count,float x,float spread,long time){int[] ids=new int[count];float[] xs=new float[count],ys=new float[count];for(int i=0;i<count;i++){ids[i]=i;xs[i]=x+(i-1)*spread;ys[i]=220;}t.fingers(ids,xs,ys,time);}
 public static void main(String[] args){
  Sink s=new Sink();MissionTouch t=new MissionTouch(s);
  tap(t,200,200,0);check(s.clicks.equals(Arrays.asList("200,200")));tap(t,200,200,100);check(s.clicks.size()==2); // No double-tap requirement.
  int before=s.clicks.size();t.down(100,100,500);t.move(108,108);check(s.presses.isEmpty());t.move(130,130);check(s.presses.equals(Arrays.asList("100,100")));t.move(200,220);t.up(200,220,700);check(s.releases.equals(Arrays.asList("200,220"))&&s.clicks.size()==before);
  t.down(100,100,800);t.move(150,160);t.cancel();check(s.releases.size()==2);t.cancel();check(s.releases.size()==2);t.up(150,160,900);check(s.clicks.size()==before);
  t.down(200,200,1000);check(!t.hold(1449));check(t.hold(1450));check(!t.hold(1500));t.up(200,200,1550);check(s.clicks.size()==before+1);
  before=s.clicks.size();t.aim(true);tap(t,250,250,2000);check(s.clicks.size()==before);t.down(250,250,2100);check(t.hold(2550));t.up(250,250,2600);check(s.clicks.size()==before+1&&s.clicks.get(before).equals("250,250"));
  before=s.clicks.size();int presses=s.presses.size();t.down(200,200,3000);t.move(300,300);check(!t.hold(4000));t.up(300,300,4100);check(s.clicks.size()==before&&s.presses.size()==presses);
  t.down(210,220,4200);t.up(210,220,4700);check(s.clicks.size()==before+1); // Hold recognized even without timer delivery.
  before=s.clicks.size();t.down(220,220,4800);t.click();t.up(220,220,4900);check(s.clicks.size()==before+1);
  t.aim(false);before=s.clicks.size();t.down(200,200,5000);fingers(t,2,250,40,5010);fingers(t,2,280,40,5100);check(s.actions.equals(Arrays.asList(5)));check(Math.abs(s.pans.get(0)+3.6f)<.01f);check(!t.hold(5500));t.pairEnd(5510);t.move(220,220);t.up(220,220,5600);check(s.clicks.size()==before);
  s.actions.clear();t.down(200,200,6000);t.move(240,240);int releases=s.releases.size();fingers(t,2,250,40,6100);check(s.releases.size()==releases+1);t.pairEnd(6150);t.up(250,220,6200);check(s.clicks.size()==before);
  t.cancel();s.actions.clear();t.fingers(new int[]{1,2},new float[]{200,300},new float[]{200,200},6300);t.fingers(new int[]{2,1},new float[]{340,160},new float[]{200,200},6400);check(s.actions.isEmpty()); // Spread is not zoom.
  t.cancel();t.aim(true);s.actions.clear();fingers(t,3,300,30,7000);t.fingers(new int[]{2,0,1},new float[]{330,270,300},new float[]{220,220,220},7100);t.pairEnd(7150);check(s.actions.equals(Arrays.asList(3)));t.pairEnd(7160);t.up(300,220,7200);check(s.actions.size()==1&&s.clicks.size()==before);
  t.cancel();s.actions.clear();fingers(t,3,300,30,8000);fingers(t,3,340,30,8100);t.pairEnd(8150);check(s.actions.isEmpty()); // Swipe is no longer rotation.
  t.cancel();s.actions.clear();fingers(t,3,300,30,8200);fingers(t,3,300,50,8250);t.pairEnd(8300);check(s.actions.isEmpty()); // Individual motion cancels even with stationary centroid.
  t.cancel();s.actions.clear();fingers(t,3,300,30,9000);t.pairEnd(9400);check(s.actions.isEmpty());
  t.cancel();fingers(t,3,300,30,9500);t.pairEnd(9850);check(s.actions.equals(Arrays.asList(3)));
  t.cancel();s.actions.clear();t.aim(false);fingers(t,3,300,30,10000);t.pairEnd(10100);check(s.actions.isEmpty());
  t.cancel();t.aim(true);fingers(t,4,300,30,11000);fingers(t,3,300,30,11050);t.pairEnd(11100);check(s.actions.isEmpty());
  t.cancel();fingers(t,2,300,30,12000);fingers(t,2,340,30,12050);s.actions.clear();fingers(t,3,340,30,12100);t.pairEnd(12200);check(s.actions.isEmpty());
  t.cancel();fingers(t,3,300,30,13000);t.fingers(new int[]{0,1,4},new float[]{270,300,330},new float[]{220,220,220},13050);t.pairEnd(13100);check(s.actions.isEmpty());
  t.cancel();fingers(t,3,300,30,14000);t.cancel();t.pairEnd(14100);check(s.actions.isEmpty());
  before=s.clicks.size();t.aim(false);t.down(-5,-5,15000);check(!t.hold(16000));t.up(5,5,16100);check(s.clicks.size()==before);t.down(100,100,17000);check(!t.hold(16900));t.up(100,100,16900);check(s.clicks.size()==before);
  t.down(100,100,18000);t.move(200,200);int released=s.releases.size();t.aim(true);check(s.releases.size()==released+1);t.up(200,200,18100);check(s.clicks.size()==before);
  t.cancel();s.actions.clear();t.fingers(new int[]{1,1,2},new float[]{200,300,400},new float[]{200,200,200},19000);t.pairEnd(19100);check(s.actions.isEmpty());
  System.out.println("Tap left-click, held mouse drag selection, one-shot hold placement, three-finger taps, pan without zoom, cancellation and pointer identity PASS ("+checks+" checks)");
 }
}
