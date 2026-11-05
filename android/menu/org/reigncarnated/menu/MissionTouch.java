package org.reigncarnated.menu;

/** Left mouse outside placement; anchored tap/rotate/double-tap while placing. */
final class MissionTouch {
 static final long HOLD_MS=450, DOUBLE_MS=300;
 private static final float SLOP=12, DOUBLE_SLOP=32;
 interface Sink {
  void move(int x,int y);void press(int x,int y);void release(int x,int y);
  void click(int x,int y);void camera(int action,float x,float y);
 }
 private final Sink sink;
 private boolean active,multi,aim,moved,selecting,held,pairValid,anchored,tapEligible,pendingRotation;
 private int idA,idB,cursorX=320,cursorY=240;
 private float downX,downY,centerX,centerY,tapX,tapY;
 private long downTime,lastTap;
 MissionTouch(Sink sink){this.sink=sink;}
 private static boolean inside(float x,float y){return Float.isFinite(x)&&Float.isFinite(y)&&x>=0&&x<640&&y>=0&&y<480;}
 private void cursor(float x,float y){if(inside(x,y)){cursorX=(int)x;cursorY=(int)y;sink.move(cursorX,cursorY);}}
 private void releaseSelection(){if(selecting){sink.release(cursorX,cursorY);selecting=false;}}
 private void clearTap(){pendingRotation=tapEligible=false;}
 void aim(boolean enabled){cancel();aim=enabled;anchored=false;}
 void down(float x,float y,long time){
  releaseSelection();advance(time);active=true;multi=moved=held=pairValid=false;downX=x;downY=y;downTime=time;
  // An anchored tap must never move the native pointer, even on DOWN.
  if(!aim||!anchored)cursor(x,y);
 }
 void move(float x,float y){
  if(!active||multi)return;
  if(Math.hypot(x-downX,y-downY)>=SLOP){
   moved=true;clearTap();
   if(!aim&&!selecting&&!held&&inside(downX,downY)){selecting=true;sink.press((int)downX,(int)downY);}
  }
  if(!aim||!anchored||moved)cursor(x,y);
 }
 boolean hold(long time){
  if(aim||!active||multi||moved||held||!inside(downX,downY)||time<downTime||time-downTime<HOLD_MS)return false;
  held=true;sink.click(cursorX,cursorY);return true;
 }
 void advance(long time){
  if(tapEligible&&(time<lastTap||time-lastTap>DOUBLE_MS)){
   if(pendingRotation)sink.camera(3,0,0);clearTap();
  }
 }
 long rotationDeadline(){return pendingRotation?lastTap+DOUBLE_MS+1:0;}
 private void placementTap(float x,float y,long time){
  if(!anchored){cursor(x,y);anchored=true;clearTap();}
  else if(tapEligible&&time>=lastTap&&time-lastTap<=DOUBLE_MS&&Math.hypot(x-tapX,y-tapY)<=DOUBLE_SLOP){
   clearTap();sink.click(cursorX,cursorY);return;
  }else{
   if(pendingRotation)sink.camera(3,0,0);
   pendingRotation=true;
  }
  lastTap=time;tapX=x;tapY=y;tapEligible=true;
 }
 void up(float x,float y,long time){
  advance(time);
  if(selecting){cursor(x,y);releaseSelection();}
  else if(active&&!multi&&!held&&inside(downX,downY)&&inside(x,y)&&time>=downTime){
   if(aim){
    if(moved){cursor(x,y);anchored=true;clearTap();}
    else if(time-downTime<HOLD_MS)placementTap(x,y,time);
    else {if(!anchored){cursor(x,y);anchored=true;}clearTap();}
   }else if(!moved){cursor(x,y);if(!hold(time))sink.click(cursorX,cursorY);}
  }
  active=multi=pairValid=false;
 }
 void click(){cancel();sink.click(cursorX,cursorY);}
 void cancel(){releaseSelection();active=multi=moved=held=pairValid=false;clearTap();}
 void pairEnd(long time){releaseSelection();multi=true;active=pairValid=false;clearTap();}
 void fingers(int[] ids,float[] xs,float[] ys,long time){
  releaseSelection();multi=true;active=false;clearTap();
  if(ids.length!=2||xs.length!=2||ys.length!=2||ids[0]==ids[1]){pairValid=false;return;}
  if(!Float.isFinite(xs[0])||!Float.isFinite(xs[1])||!Float.isFinite(ys[0])||!Float.isFinite(ys[1])){pairValid=false;return;}
  int a=Math.min(ids[0],ids[1]),b=Math.max(ids[0],ids[1]);float cx=(xs[0]+xs[1])/2,cy=(ys[0]+ys[1])/2;
  if(!pairValid||idA!=a||idB!=b){idA=a;idB=b;centerX=cx;centerY=cy;pairValid=true;return;}
  float dx=centerX-cx,dy=cy-centerY;centerX=cx;centerY=cy;
  if(dx!=0||dy!=0)sink.camera(5,dx*.12f,dy*.12f);
  // Three/four fingers, pinch and twist never click, place or rotate a building.
 }
}
