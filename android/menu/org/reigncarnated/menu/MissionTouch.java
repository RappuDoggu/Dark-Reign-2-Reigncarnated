package org.reigncarnated.menu;

/** Touch coordinates are in the original 640 x 480 engine viewport. */
final class MissionTouch {
 interface Sink {
  void move(int x,int y);
  void click(int x,int y);
  void camera(int action,float x,float y);
 }
 private final Sink sink;
 private boolean active,multi,aim,moved,gestureValid;
 private int gestureCount,idA,idB,idC,cursorX=320,cursorY=240;
 private float downX,downY,centerX,centerY,buildingDrag,tapX,tapY;
 private long downTime,lastTap=-1;
 MissionTouch(Sink sink){this.sink=sink;}
 private static boolean inside(float x,float y){return x>=0&&x<640&&y>=0&&y<480;}
 private void cursor(float x,float y){if(inside(x,y)){cursorX=(int)x;cursorY=(int)y;sink.move(cursorX,cursorY);}}
 void aim(boolean enabled){aim=enabled;lastTap=-1;active=false;}
 void down(float x,float y,long time){active=true;multi=false;moved=false;gestureValid=false;downX=x;downY=y;downTime=time;cursor(x,y);}
 void move(float x,float y){if(!active||multi)return;if(Math.hypot(x-downX,y-downY)>=12){moved=true;lastTap=-1;}cursor(x,y);}
 void up(float x,float y,long time){
  if(active&&!multi&&!moved&&inside(downX,downY)&&inside(x,y)&&time>=downTime&&time-downTime<=300&&Math.hypot(x-downX,y-downY)<12){
   cursor(x,y);
   if(lastTap>=0&&time>=lastTap&&time-lastTap<=350&&Math.hypot(x-tapX,y-tapY)<=24){sink.click(cursorX,cursorY);lastTap=-1;}
   else {lastTap=time;tapX=x;tapY=y;}
  }else lastTap=-1;
  active=multi=gestureValid=false;
 }
 void click(){lastTap=-1;sink.click(cursorX,cursorY);}
 void cancel(){active=multi=gestureValid=false;lastTap=-1;}
 void pairEnd(){multi=true;active=false;gestureValid=false;lastTap=-1;}
 void fingers(int[] ids,float[] xs,float[] ys){
  multi=true;active=false;lastTap=-1;
  if(ids.length<2||ids.length>3){gestureValid=false;return;}
  int[] sorted=ids.clone();java.util.Arrays.sort(sorted);float cx=0,cy=0;
  for(int i=0;i<ids.length;i++){cx+=xs[i];cy+=ys[i];}cx/=ids.length;cy/=ids.length;
  int third=ids.length==3?sorted[2]:-1;
  if(!gestureValid||gestureCount!=ids.length||idA!=sorted[0]||idB!=sorted[1]||idC!=third){gestureCount=ids.length;idA=sorted[0];idB=sorted[1];idC=third;centerX=cx;centerY=cy;buildingDrag=0;gestureValid=true;return;}
  float dx=centerX-cx,dy=cy-centerY;centerX=cx;centerY=cy;
  if(ids.length==2){if(dx!=0||dy!=0)sink.camera(5,dx*.12f,dy*.12f);}
  else if(aim){
   buildingDrag-=dx;
   // Each deliberate horizontal three-finger swipe steps the original compass.
   int steps=0;while(Math.abs(buildingDrag)>=32&&steps++<4){boolean right=buildingDrag>0;sink.camera(right?3:2,0,0);buildingDrag+=right?-32:32;}
  }
  // No gesture can issue wheel/zoom input or rotate the map.
 }
}
