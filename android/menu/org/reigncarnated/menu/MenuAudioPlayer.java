package org.reigncarnated.menu;
import android.content.Context;
import android.media.AudioAttributes;
import android.media.AudioFocusRequest;
import android.media.AudioManager;
import android.media.SoundPool;
import java.util.HashMap;
import java.util.HashSet;
import java.util.function.Consumer;
// Main-thread ownership: asynchronous loads cannot replay after release.
final class MenuAudioPlayer {
 private final SoundPool pool;
 private final AudioManager manager;
 private final AudioFocusRequest focus;
 private final Consumer<String> report;
 private final HashMap<String,Integer> sounds=new HashMap<>();
 private final HashSet<Integer> ready=new HashSet<>();
 private final HashMap<Integer,Float> pending=new HashMap<>();
 private boolean closed,focused,musicReady;
 private android.media.MediaPlayer music;
 private float musicVolume=0.4f;
 void setMusicVolume(float v){musicVolume=Math.max(0,Math.min(1,v));if(musicReady)music.setVolume(musicVolume,musicVolume);}
 MenuAudioPlayer(Context context,Consumer<String> report){
  this.report=report;manager=(AudioManager)context.getSystemService(Context.AUDIO_SERVICE);
  AudioAttributes attributes=new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build();
  pool=new SoundPool.Builder().setMaxStreams(8).setAudioAttributes(attributes).build();
  focus=new AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN).setAudioAttributes(attributes).setOnAudioFocusChangeListener(change->{if(closed)return;focused=change==AudioManager.AUDIOFOCUS_GAIN;if(focused){pool.autoResume();if(musicReady)music.start();}else{pool.autoPause();pending.clear();if(musicReady&&music.isPlaying())music.pause();}}).build();
  pool.setOnLoadCompleteListener((p,id,status)->{if(closed)return;if(status!=0){pending.remove(id);report.accept("Menu sound failed to decode ("+status+").");return;}ready.add(id);Float volume=pending.remove(id);if(volume!=null&&focused)play(id,volume);});
 }
 void playMusic(String path){
  if(closed||path==null)return;
  if(music!=null){music.release();music=null;}musicReady=false;
  android.media.MediaPlayer player=new android.media.MediaPlayer();music=player;
  player.setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build());
  player.setLooping(true);player.setVolume(musicVolume,musicVolume);
  player.setOnPreparedListener(p->{if(closed||music!=p)return;musicReady=true;if(!focused)focused=manager.requestAudioFocus(focus)==AudioManager.AUDIOFOCUS_REQUEST_GRANTED;if(focused)p.start();else report.accept("Music is ready; audio focus is unavailable.");});
  player.setOnErrorListener((p,what,extra)->{if(music==p){musicReady=false;report.accept("Music could not play ("+what+", "+extra+").");}return true;});
  try{player.setDataSource(path);player.prepareAsync();}catch(Exception e){player.release();music=null;report.accept("Music could not load: "+e.getMessage());}
 }
 void accept(String[] events){
  if(closed||events==null)return;
  for(int i=0;i+1<events.length;i+=2){
   float volume=Float.parseFloat(events[i+1]);Integer id=sounds.get(events[i]);
   if(id==null){id=pool.load(events[i],1);if(id==0){report.accept("Menu sound could not be loaded.");continue;}sounds.put(events[i],id);}
   if(volume<0)continue;
   if(!focused){focused=manager.requestAudioFocus(focus)==AudioManager.AUDIOFOCUS_REQUEST_GRANTED;if(!focused){report.accept("Menu audio focus is unavailable.");continue;}}
   if(ready.contains(id))play(id,volume);else pending.put(id,volume);
  }
 }
 private void play(int id,float volume){if(pool.play(id,volume,volume,1,0,1)==0)report.accept("Menu sound playback could not start.");}
 void close(){if(closed)return;closed=true;pending.clear();if(music!=null){music.release();music=null;}musicReady=false;pool.release();manager.abandonAudioFocusRequest(focus);}
}
