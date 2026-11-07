package org.reigncarnated.menu;
/** Pure elapsed-time sampler. Reset lifecycle gaps; timing is native wall time. */
final class FrameTiming {
 long since,work,max;int frames;
 void reset(){since=work=max=0;frames=0;}
 boolean frame(long start,long end){
  if(since==0||end<=since||end-since>3_000_000_000L){reset();since=end;return false;}
  long cost=Math.max(0,end-start);work+=cost;max=Math.max(max,cost);++frames;
  return end-since>=1_000_000_000L;
 }
}
