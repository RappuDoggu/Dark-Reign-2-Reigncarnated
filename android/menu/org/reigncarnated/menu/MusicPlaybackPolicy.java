package org.reigncarnated.menu;
/** Shared by preparation and focus callbacks: neither may bypass mission speech. */
final class MusicPlaybackPolicy {
 private int blocked;
 private boolean closed;
 void block(int reasons){blocked=reasons;}
 void close(){closed=true;}
 boolean shouldPlay(boolean ready,boolean focused){return !closed&&blocked==0&&ready&&focused;}
}
