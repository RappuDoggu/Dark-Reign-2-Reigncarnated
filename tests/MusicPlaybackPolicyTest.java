package org.reigncarnated.menu;
public final class MusicPlaybackPolicyTest {
 private static int checks;
 private static void check(boolean v){++checks;if(!v)throw new AssertionError("Music policy check "+checks);}
 public static void main(String[] args){
  MusicPlaybackPolicy p=new MusicPlaybackPolicy();check(p.shouldPlay(true,true));
  p.block(8);check(!p.shouldPlay(false,false));check(!p.shouldPlay(true,true)); // Late prepare/focus while loading.
  p.block(1);check(!p.shouldPlay(true,true));p.block(1|4);check(!p.shouldPlay(true,true));
  p.block(4);check(!p.shouldPlay(true,true)); // Cinematic ends before its last dialogue finishes.
  p.block(0);check(!p.shouldPlay(true,false));check(p.shouldPlay(true,true));
  p.block(2);check(!p.shouldPlay(true,true));p.block(0);check(!p.shouldPlay(false,true));
  p.block(8);check(!p.shouldPlay(true,true));p.block(0);check(p.shouldPlay(true,true)); // Next-mission handoff.
  p.close();check(!p.shouldPlay(true,true));p.block(0);check(!p.shouldPlay(true,true));
  System.out.println(checks+" music preparation/focus/cinematic/speech/movie/handoff/close checks PASS");
 }
}
