package org.reigncarnated.menu;

/** Touch coordinates are in the original 640 x 480 engine viewport. */
final class MissionTouch {
 interface Sink {
  void move(int x,int y);
  void press(int x,int y);
  void release(int x,int y);
  void click(int x,int y);
  void camera(int action,float x,float y);
 }
 private final Sink sink;
 private boolean aim,multi,selecting,pairValid,active;
 private float downX,downY,centerX,centerY,angle,turn,panX,panY;
 private int firstId,secondId,mode;
 private int cursorX=320,cursorY=240;
 MissionTouch(Sink sink){this.sink=sink;}
 private static boolean inside(float x,float y){return x>=0&&x<640&&y>=0&&y<480;}
 private void cursor(float x,float y){if(inside(x,y)){cursorX=(int)x;cursorY=(int)y;sink.move(cursorX,cursorY);}}
 void aim(boolean enabled){if(selecting){sink.release(cursorX,cursorY);selecting=false;}active=false;aim=enabled;}
 void down(float x,float y){cancel();active=true;downX=x;downY=y;cursor(x,y);}
 void move(float x,float y){
  if(!active||multi)return;
  if(!aim&&!selecting&&inside(downX,downY)&&Math.hypot(x-downX,y-downY)>=12){selecting=true;sink.press((int)downX,(int)downY);}
  cursor(x,y);
 }
 void up(float x,float y){
  if(selecting){sink.release((int)x,(int)y);selecting=false;}
  else if(active&&!multi&&!aim&&inside(downX,downY)&&inside(x,y)&&Math.hypot(x-downX,y-downY)<12){cursor(x,y);sink.click(cursorX,cursorY);}
  active=multi=pairValid=false;mode=0;
 }
 void click(){sink.click(cursorX,cursorY);}
 void cancel(){if(selecting){sink.release(cursorX,cursorY);selecting=false;}active=multi=pairValid=false;mode=0;}
 void pairEnd(){multi=true;pairValid=false;mode=0;}
 void pair(int a,float ax,float ay,int b,float bx,float by){
  if(selecting){sink.release(cursorX,cursorY);selecting=false;}
  multi=true;active=false;
  // Keep the vector orientation stable if Android reorders its pointer indices.
  if(a>b){int id=a;a=b;b=id;float f=ax;ax=bx;bx=f;f=ay;ay=by;by=f;}
  float cx=(ax+bx)*.5f,cy=(ay+by)*.5f;
  if(Math.hypot(bx-ax,by-ay)<8){pairValid=false;mode=0;return;}
  float next=(float)Math.toDegrees(Math.atan2(by-ay,bx-ax));
  if(!pairValid||a!=firstId||b!=secondId){firstId=a;secondId=b;centerX=cx;centerY=cy;angle=next;turn=panX=panY=0;mode=0;pairValid=true;return;}
  float delta=next-angle;while(delta>180)delta-=360;while(delta< -180)delta+=360;
  float dx=centerX-cx,dy=cy-centerY;angle=next;centerX=cx;centerY=cy;
  if(mode==0){
   turn+=delta;panX+=dx;panY+=dy;
   // Lock one gesture until the fingers lift: twisting cannot also pan or zoom.
   if(Math.abs(turn)>=3){mode=2;sink.camera(6,turn,0);}
   else if(Math.hypot(panX,panY)>=8){mode=1;sink.camera(5,panX*.12f,panY*.12f);}
  }else if(mode==2){if(delta!=0)sink.camera(6,delta,0);}
  else if(dx!=0||dy!=0)sink.camera(5,dx*.12f,dy*.12f);
  // Distance between the fingers never generates wheel/zoom input.
 }
}
