package org.reigncarnated.menu;
import java.util.*;
public final class MissionTouchTest {
 static int checks;
 static void check(boolean b){checks++;if(!b)throw new AssertionError("touch check "+checks);}
 static final class Sink implements MissionTouch.Sink {
  List<String> mouse=new ArrayList<>();List<Integer> actions=new ArrayList<>();float rotation;
  public void move(int x,int y){mouse.add("move "+x+" "+y);}
  public void press(int x,int y){mouse.add("press "+x+" "+y);}
  public void release(int x,int y){mouse.add("release "+x+" "+y);}
  public void click(int x,int y){mouse.add("click "+x+" "+y);}
  public void camera(int a,float x,float y){actions.add(a);if(a==6)rotation+=x;check(a!=4);}
 }
 static void pair(MissionTouch t,float degrees,float radius){double a=Math.toRadians(degrees);float x=(float)Math.cos(a)*radius,y=(float)Math.sin(a)*radius;t.pair(2,320-x,240-y,9,320+x,240+y);}
 public static void main(String[] args){
  Sink s=new Sink();MissionTouch t=new MissionTouch(s);
  t.down(200,200);t.up(200,200);check(s.mouse.contains("click 200 200"));
  s.mouse.clear();t.down(200,200);t.move(230,240);t.up(230,240);check(s.mouse.contains("press 200 200")&&s.mouse.contains("release 230 240"));check(s.mouse.stream().noneMatch(x->x.startsWith("click")));
  s.mouse.clear();t.aim(true);t.down(200,200);t.move(260,280);t.up(260,280);check(s.mouse.stream().noneMatch(x->x.startsWith("press")||x.startsWith("click")));t.click();check(s.mouse.contains("click 260 280"));t.aim(false);
  s.mouse.clear();t.down(200,200);pair(t,0,60);pair(t,30,60);pair(t,60,60);check(Math.abs(s.rotation-60)<.01f);check(s.actions.stream().allMatch(a->a==6));t.pairEnd();t.move(220,220);t.up(220,220);check(s.mouse.stream().noneMatch(x->x.startsWith("click")||x.startsWith("press")));
  s.actions.clear();s.rotation=0;t.down(200,200);pair(t,0,60);pair(t,-30,60);check(Math.abs(s.rotation+30)<.01f);t.up(200,200);
  s.actions.clear();s.rotation=0;t.down(200,200);pair(t,179,60);pair(t,-177,60);check(Math.abs(s.rotation-4)<.01f);t.up(200,200);
  s.actions.clear();t.down(200,200);pair(t,0,60);pair(t,0,100);pair(t,0,30);check(s.actions.isEmpty());t.up(200,200);
  s.actions.clear();t.down(200,200);t.pair(2,200,200,9,300,200);t.pair(9,320,220,2,220,220);check(s.actions.equals(Arrays.asList(5)));t.pair(2,225,200,9,315,240);check(s.actions.stream().allMatch(a->a==5));t.up(200,200);
  s.actions.clear();t.down(200,200);pair(t,0,60);t.pair(3,320,180,9,320,300);check(s.actions.isEmpty());t.up(200,200);
  s.actions.clear();s.rotation=0;t.down(200,200);pair(t,0,60);pair(t,0,1);pair(t,90,60);check(s.actions.isEmpty());pair(t,100,60);check(Math.abs(s.rotation-10)<.01f);t.cancel();
  s.mouse.clear();t.down(-5,-5);t.up(10,10);check(s.mouse.stream().noneMatch(x->x.startsWith("click")));
  s.mouse.clear();t.down(100,100);t.move(140,140);t.cancel();check(s.mouse.contains("release 140 140"));
  s.mouse.clear();t.down(100,100);t.cancel();t.up(100,100);check(s.mouse.stream().noneMatch(x->x.startsWith("click")));t.up(100,100);check(s.mouse.stream().noneMatch(x->x.startsWith("click")));
  s.mouse.clear();s.actions.clear();t.down(200,200);pair(t,0,60);pair(t,10,60);t.aim(true);pair(t,20,60);t.up(200,200);check(s.actions.equals(Arrays.asList(6,6)));check(s.mouse.stream().noneMatch(x->x.startsWith("click")));t.aim(false);
  System.out.println("Mission touch: taps, drag, aim/click, rotation both ways, angle wrap, pinch without zoom, pan lock, pointer changes and cancel PASS ("+checks+" checks)");
 }
}
