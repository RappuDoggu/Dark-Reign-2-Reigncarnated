package org.reigncarnated.menu;

/** Original left-mouse drag selection; placement confirmation requires a hold. */
final class MissionTouch {
 static final long HOLD_MS=450, THREE_TAP_MS=350;
 private static final float SLOP=12;
 interface Sink {
  void move(int x,int y);
  void press(int x,int y);
  void release(int x,int y);
  void click(int x,int y);
  void camera(int action,float x,float y);
 }
 private final Sink sink;
 private boolean active,multi,aim,moved,selecting,held,pairValid,threeValid,threeBlocked,multiMoved;
 private int gestureCount,idA,idB,idC,cursorX=320,cursorY=240;
 private float downX,downY,centerX,centerY,panTravel;
 private final float[] threeX=new float[3],threeY=new float[3];
 private long downTime,threeTime;
 MissionTouch(Sink sink){this.sink=sink;}
 private static boolean inside(float x,float y){return Float.isFinite(x)&&Float.isFinite(y)&&x>=0&&x<640&&y>=0&&y<480;}
 private void cursor(float x,float y){if(inside(x,y)){cursorX=(int)x;cursorY=(int)y;sink.move(cursorX,cursorY);}}
 private void releaseSelection(){if(selecting){sink.release(cursorX,cursorY);selecting=false;}}
 void aim(boolean enabled){releaseSelection();active=false;aim=enabled;}
 void down(float x,float y,long time){cancel();active=true;downX=x;downY=y;downTime=time;cursor(x,y);}
 void move(float x,float y){
  if(!active||multi)return;
  if(Math.hypot(x-downX,y-downY)>=SLOP){moved=true;if(!aim&&!selecting&&!held&&inside(downX,downY)){selecting=true;sink.press((int)downX,(int)downY);}}
  cursor(x,y);
 }
 boolean hold(long time){
  if(!active||multi||moved||held||!inside(downX,downY)||time<downTime||time-downTime<HOLD_MS)return false;
  held=true;sink.click(cursorX,cursorY);return true;
 }
 void up(float x,float y,long time){
  if(selecting){cursor(x,y);releaseSelection();}
  else if(active&&!multi&&!held&&!moved&&inside(downX,downY)&&inside(x,y)&&time>=downTime){cursor(x,y);if(!hold(time)&&!aim)sink.click(cursorX,cursorY);}
  active=multi=pairValid=threeValid=threeBlocked=multiMoved=false;gestureCount=0;
 }
 void click(){cancel();sink.click(cursorX,cursorY);}
 void cancel(){releaseSelection();active=multi=moved=held=pairValid=threeValid=threeBlocked=multiMoved=false;gestureCount=0;panTravel=0;}
 void pairEnd(long time){
  if(gestureCount==3&&threeValid&&!threeBlocked&&!multiMoved&&aim&&time>=threeTime&&time-threeTime<=THREE_TAP_MS){sink.camera(3,0,0);threeBlocked=true;}
  if(gestureCount>=3)threeBlocked=true;
  releaseSelection();multi=true;active=pairValid=threeValid=false;gestureCount=0;
 }
 void fingers(int[] ids,float[] xs,float[] ys,long time){
  releaseSelection();multi=true;active=false;
  if(ids.length<2||ids.length>3||xs.length!=ids.length||ys.length!=ids.length){pairValid=threeValid=false;threeBlocked=true;return;}
  int[] sorted=ids.clone();java.util.Arrays.sort(sorted);float cx=0,cy=0;
  for(int i=0;i<ids.length;i++){if(!Float.isFinite(xs[i])||!Float.isFinite(ys[i])||(i>0&&sorted[i]==sorted[i-1])){pairValid=threeValid=false;threeBlocked=true;return;}cx+=xs[i];cy+=ys[i];}cx/=ids.length;cy/=ids.length;
  int third=ids.length==3?sorted[2]:-1;
  boolean same=pairValid&&gestureCount==ids.length&&idA==sorted[0]&&idB==sorted[1]&&idC==third;
  if(!same){
   if(pairValid&&gestureCount==3)threeBlocked=true;
   gestureCount=ids.length;idA=sorted[0];idB=sorted[1];idC=third;centerX=cx;centerY=cy;pairValid=true;
   if(ids.length==3){threeTime=time;threeValid=!threeBlocked&&!multiMoved;for(int j=0;j<3;j++)for(int i=0;i<3;i++)if(ids[i]==sorted[j]){threeX[j]=xs[i];threeY[j]=ys[i];}}
   return;
  }
  float dx=centerX-cx,dy=cy-centerY;centerX=cx;centerY=cy;
  if(ids.length==2){panTravel+=Math.hypot(dx,dy);if(panTravel>=SLOP)multiMoved=true;if(dx!=0||dy!=0)sink.camera(5,dx*.12f,dy*.12f);}
  else for(int j=0;j<3;j++)for(int i=0;i<3;i++)if(ids[i]==sorted[j]&&Math.hypot(xs[i]-threeX[j],ys[i]-threeY[j])>=SLOP)threeValid=false;
  // No pinch, wheel, twist or swipe rotation path exists.
 }
}
