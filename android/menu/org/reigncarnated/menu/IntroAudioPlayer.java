package org.reigncarnated.menu;
import android.content.Context;
import android.media.*;
import java.util.concurrent.*;
import java.util.function.Consumer;
/** A bounded worker owns decoded original Bink PCM; UI and GL threads never block on writes. */
final class IntroAudioPlayer {
 private final ArrayBlockingQueue<short[]> queue=new ArrayBlockingQueue<>(32);
 private final AudioTrack track;
 private final AudioManager manager;
 private final AudioFocusRequest focus;
 private final Thread worker;
 private volatile boolean closed;
 final int movie,rate;
 IntroAudioPlayer(Context context,int movie,int rate,Consumer<String> report){
  this.movie=movie;this.rate=rate;manager=(AudioManager)context.getSystemService(Context.AUDIO_SERVICE);
  AudioAttributes attributes=new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_MOVIE).build();
  int buffer=Math.max(AudioTrack.getMinBufferSize(rate,AudioFormat.CHANNEL_OUT_STEREO,AudioFormat.ENCODING_PCM_16BIT),rate/5*4);
  track=new AudioTrack.Builder().setAudioAttributes(attributes).setAudioFormat(new AudioFormat.Builder().setSampleRate(rate).setChannelMask(AudioFormat.CHANNEL_OUT_STEREO).setEncoding(AudioFormat.ENCODING_PCM_16BIT).build()).setBufferSizeInBytes(buffer).setTransferMode(AudioTrack.MODE_STREAM).build();
  focus=new AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN).setAudioAttributes(attributes).setOnAudioFocusChangeListener(change->{if(closed)return;try{if(change==AudioManager.AUDIOFOCUS_GAIN)track.play();else track.pause();}catch(IllegalStateException ignored){}}).build();
  if(manager.requestAudioFocus(focus)==AudioManager.AUDIOFOCUS_REQUEST_GRANTED)track.play();
  worker=new Thread(()->{try{while(!closed){short[] pcm=queue.take();int at=0;while(at<pcm.length&&!closed){int n=track.write(pcm,at,pcm.length-at,AudioTrack.WRITE_BLOCKING);if(n<0)throw new IllegalStateException("AudioTrack write "+n);at+=n;}}}catch(InterruptedException ignored){}catch(Exception e){if(!closed)report.accept("Intro sound: "+e.getMessage());}finally{closed=true;manager.abandonAudioFocusRequest(focus);track.release();}},"ReignIntroPCM");worker.start();
 }
 void accept(short[] pcm){if(!closed&&pcm!=null&&pcm.length>0&&!queue.offer(pcm))throw new IllegalStateException("Intro audio queue overflow");}
 void close(){if(closed)return;closed=true;queue.clear();try{track.pause();track.flush();}catch(IllegalStateException ignored){}worker.interrupt();manager.abandonAudioFocusRequest(focus);}
}
