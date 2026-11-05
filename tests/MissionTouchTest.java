package org.reigncarnated.menu;
import java.util.*;
public final class MissionTouchTest {
 static int checks;
 static void check(boolean b){checks++;if(!b)throw new AssertionError("touch check "+checks);}
 static final class Sink implements MissionTouch.Sink {
  List<String> moves=new ArrayList<>(),clicks=new ArrayList<>(),presses=new ArrayList<>(),releases=new ArrayList<>();List<Integer> actions=new ArrayList<>();List<Float> pans=new ArrayList<>();
  public void move(int x,int y){moves.add(x+","+y);}public void press(int x,int y){presses.add(x+","+y);}public void release(int x,int y){releases.add(x+","+y);}public void click(int x,int y){clicks.add(x+","+y);}
  public void camera(int a,float x,float y){check(a==3||a==5);actions.add(a);if(a==5)pans.add(x);}
 }
 static void tap(MissionTouch t,float x,float y,long time){t.down(x,y,time);t.up(x,y,time+40);}
 static void pair(MissionTouch t,float x,float spread,long time){t.fingers(new int[]{0,1},new float[]{x-spread,x+spread},new float[]{220,220},time);}
 public static void main(String[] args){
  Sink s=new Sink();MissionTouch t=new MissionTouch(s);
  tap(t,200,200,0);check(s.clicks.equals(Arrays.asList("200,200")));tap(t,200,200,100);check(s.clicks.size()==2);
  int before=s.clicks.size();t.down(100,100,500);t.move(108,108);check(s.presses.isEmpty());t.move(130,130);check(s.presses.equals(Arrays.asList("100,100")));t.move(200,220);t.up(200,220,700);check(s.releases.equals(Arrays.asList("200,220"))&&s.clicks.size()==before);
  t.down(100,100,800);t.move(150,160);t.cancel();check(s.releases.size()==2);t.cancel();check(s.releases.size()==2);t.up(150,160,900);check(s.clicks.size()==before);
  t.down(200,200,1000);check(!t.hold(1449));check(t.hold(1450));check(!t.hold(1500));t.up(200,200,1550);check(s.clicks.size()==before+1);
  // Initial placement tap anchors immediately. Rotation taps never move the cursor.
  t.aim(true);before=s.clicks.size();tap(t,250,250,2000);check(s.clicks.size()==before&&t.rotationDeadline()==0);
  t.advance(2400);int moves=s.moves.size();s.actions.clear();tap(t,420,320,2500);check(s.moves.size()==moves&&s.clicks.size()==before&&s.actions.isEmpty());
  long deadline=t.rotationDeadline();check(deadline==2841);t.advance(2840);check(s.actions.isEmpty());t.advance(deadline);check(s.actions.equals(Arrays.asList(3))&&s.moves.size()==moves);t.advance(5000);check(s.actions.size()==1&&t.rotationDeadline()==0);
  // A double tap cancels pending single rotation and confirms at the anchor,
  // even when the fingers touch a different part of the screen.
  s.actions.clear();tap(t,500,400,5100);check(s.actions.isEmpty());tap(t,503,403,5240);check(s.clicks.get(s.clicks.size()-1).equals("250,250"));check(s.clicks.size()==before+1&&s.actions.isEmpty()&&s.moves.size()==moves);t.advance(6000);check(s.actions.isEmpty());
  // A double tap can anchor and confirm a fresh preview without a rotation.
  t.aim(true);before=s.clicks.size();tap(t,100,110,6100);tap(t,101,111,6200);check(s.clicks.size()==before+1&&s.clicks.get(before).equals("100,110"));check(t.rotationDeadline()==0);
  // Widely separated taps are independent rotation taps, not a confirmation.
  t.aim(true);tap(t,200,210,7000);t.advance(7400);s.actions.clear();before=s.clicks.size();moves=s.moves.size();tap(t,400,300,7500);tap(t,550,400,7600);check(s.actions.equals(Arrays.asList(3))&&s.clicks.size()==before&&s.moves.size()==moves);t.advance(7941);check(s.actions.equals(Arrays.asList(3,3)));
  // Drag deliberately repositions; a following single tap rotates at the new anchor.
  s.actions.clear();t.down(300,300,8100);t.move(305,305);check(s.moves.size()==moves);t.move(340,360);t.up(350,370,8400);check(s.moves.get(s.moves.size()-1).equals("350,370"));check(s.actions.isEmpty()&&s.clicks.size()==before&&t.rotationDeadline()==0);
  moves=s.moves.size();tap(t,450,400,8500);t.advance(8841);check(s.moves.size()==moves&&s.actions.equals(Arrays.asList(3)));
  // Placement holds never confirm. The normal-mode hold still chooses a build control.
  before=s.clicks.size();t.down(100,100,9000);check(!t.hold(9450));t.up(100,100,9500);check(s.clicks.size()==before&&t.rotationDeadline()==0&&s.moves.size()==moves);
  t.aim(false);t.down(210,220,9600);t.up(210,220,10100);check(s.clicks.size()==before+1);
  // Cancelling, changing placement mode or using the explicit click button cancels delayed rotation.
  t.aim(true);tap(t,200,200,11000);t.advance(11400);s.actions.clear();tap(t,300,300,11500);t.cancel();t.advance(12000);check(s.actions.isEmpty());
  tap(t,300,300,12100);t.aim(false);t.advance(13000);check(s.actions.isEmpty());
  t.aim(true);tap(t,200,200,13100);t.advance(13500);tap(t,300,300,13600);before=s.clicks.size();t.click();t.advance(14000);check(s.actions.isEmpty()&&s.clicks.size()==before+1&&s.clicks.get(before).equals("200,200"));
  // Multitouch never moves the placement cursor or causes placement/rotation on lift.
  t.aim(true);tap(t,230,240,15000);t.advance(15400);moves=s.moves.size();before=s.clicks.size();t.down(500,400,15500);t.fingers(new int[]{0,1,2},new float[]{100,300,500},new float[]{100,200,300},15510);t.pairEnd(15600);t.up(500,400,15620);t.advance(16000);check(s.moves.size()==moves&&s.clicks.size()==before&&s.actions.isEmpty());
  // Two-finger panning is retained without pinch zoom and respects pointer identity.
  t.aim(false);t.down(200,200,17000);pair(t,250,40,17010);pair(t,280,40,17100);check(s.actions.equals(Arrays.asList(5)));check(Math.abs(s.pans.get(0)+3.6f)<.01f);check(!t.hold(17500));t.pairEnd(17510);t.move(220,220);t.up(220,220,17600);check(s.clicks.size()==before);
  t.cancel();s.actions.clear();t.fingers(new int[]{1,2},new float[]{200,300},new float[]{200,200},18000);t.fingers(new int[]{2,1},new float[]{340,160},new float[]{200,200},18100);check(s.actions.isEmpty());
  t.cancel();t.down(200,200,19000);t.move(240,240);int released=s.releases.size();pair(t,250,40,19100);check(s.releases.size()==released+1);t.pairEnd(19200);t.up(250,220,19300);check(s.clicks.size()==before);
  before=s.clicks.size();t.down(-5,-5,20000);check(!t.hold(21000));t.up(5,5,21100);check(s.clicks.size()==before);t.down(100,100,22000);check(!t.hold(21900));t.up(100,100,21900);check(s.clicks.size()==before);
  t.down(100,100,23000);t.move(200,200);released=s.releases.size();t.aim(true);check(s.releases.size()==released+1);t.up(200,200,23100);check(s.clicks.size()==before);
  System.out.println("Anchored tap rotation, double-tap confirmation without pointer movement, deliberate reposition, cancellation, normal left-mouse drag/hold and pan without zoom PASS ("+checks+" checks)");
 }
}
