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
 private android.media.MediaPlayer music,ambient;
 private boolean ambientReady;
 private float musicVolume=0.4f;
 private final MusicPlaybackPolicy musicPolicy=new MusicPlaybackPolicy();
 private int musicBlockReasons;
 void setMusicBlockReasons(int reasons){if(closed||musicBlockReasons==reasons)return;musicBlockReasons=reasons;musicPolicy.block(reasons);Diagnostics.event("Mission music block="+reasons+" (cinematic=1 movie=2 voice=4 loading=8)");applyMusicState();}
 private void applyMusicState(){if(closed||!musicReady)return;if(musicPolicy.shouldPlay(musicReady,focused)){if(!music.isPlaying())music.start();}else if(music.isPlaying())music.pause();}
 private java.util.List<String> playlist=java.util.Collections.emptyList();
 private int track,failedTracks;
 void playPlaylist(java.util.List<String> paths){playlist=new java.util.ArrayList<>(paths);track=0;failedTracks=0;Diagnostics.event("Music playlist: "+playlist.size()+" tracks; volume="+musicVolume);if(!playlist.isEmpty())playMusic(playlist.get(0));}
 private void nextTrack(){if(!closed&&!playlist.isEmpty()){track=(track+1)%playlist.size();playMusic(playlist.get(track));}}
 void setMusicVolume(float v){musicVolume=Math.max(0,Math.min(1,v));if(musicReady)music.setVolume(musicVolume,musicVolume);}
 MenuAudioPlayer(Context context,Consumer<String> report){
  this.report=report;manager=(AudioManager)context.getSystemService(Context.AUDIO_SERVICE);Diagnostics.event("System music volume="+manager.getStreamVolume(AudioManager.STREAM_MUSIC)+"/"+manager.getStreamMaxVolume(AudioManager.STREAM_MUSIC));
  AudioAttributes attributes=new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build();
  pool=new SoundPool.Builder().setMaxStreams(8).setAudioAttributes(attributes).build();
  focus=new AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN).setAudioAttributes(attributes).setOnAudioFocusChangeListener(change->{if(closed)return;Diagnostics.event("Audio focus change="+change);focused=change==AudioManager.AUDIOFOCUS_GAIN;if(focused){pool.autoResume();if(ambientReady)ambient.start();}else{pool.autoPause();pending.clear();if(ambientReady&&ambient.isPlaying())ambient.pause();}applyMusicState();}).build();
  pool.setOnLoadCompleteListener((p,id,status)->{if(closed)return;if(status!=0){pending.remove(id);report.accept("Menu sound failed to decode ("+status+").");return;}ready.add(id);Float volume=pending.remove(id);if(volume!=null&&focused)play(id,volume);});
 }
 void playAmbient(String path){
  if(closed||path==null)return;if(ambient!=null)ambient.release();ambientReady=false;
  android.media.MediaPlayer player=new android.media.MediaPlayer();ambient=player;player.setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build());player.setLooping(true);
  player.setOnPreparedListener(p->{if(closed||ambient!=p)return;ambientReady=true;if(!focused)focused=manager.requestAudioFocus(focus)==AudioManager.AUDIOFOCUS_REQUEST_GRANTED;if(focused)p.start();});
  player.setOnErrorListener((p,w,e)->{report.accept("Ambient sound decode failed ("+w+", "+e+").");return true;});
  try{player.setDataSource(path);player.prepareAsync();}catch(Exception e){player.release();ambient=null;report.accept("Ambient sound: "+e.getMessage());}
 }
 void playMusic(String path){
  if(closed||path==null)return;
  if(music!=null){music.release();music=null;}musicReady=false;
  android.media.MediaPlayer player=new android.media.MediaPlayer();music=player;
  player.setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build());
  player.setLooping(false);player.setOnCompletionListener(p->{if(!closed&&music==p){failedTracks=0;nextTrack();}});player.setVolume(musicVolume,musicVolume);
  player.setOnPreparedListener(p->{if(closed||music!=p)return;musicReady=true;if(!focused)focused=manager.requestAudioFocus(focus)==AudioManager.AUDIOFOCUS_REQUEST_GRANTED;Diagnostics.event("Music prepared: duration="+p.getDuration()+"ms volume="+musicVolume);applyMusicState();Diagnostics.event("Music ready; playing="+p.isPlaying()+" block="+musicBlockReasons);if(!focused)report.accept("Music is ready; audio focus is unavailable.");});
  player.setOnErrorListener((p,what,extra)->{if(music==p){musicReady=false;report.accept("Music track could not play ("+what+", "+extra+").");if(++failedTracks<playlist.size())new android.os.Handler(android.os.Looper.getMainLooper()).post(this::nextTrack);}return true;});
  try{player.setDataSource(path);player.prepareAsync();}catch(Exception e){player.release();music=null;report.accept("Music could not load: "+e.getMessage());if(++failedTracks<playlist.size())new android.os.Handler(android.os.Looper.getMainLooper()).post(this::nextTrack);}
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
 void close(){if(closed)return;closed=true;musicPolicy.close();pending.clear();if(ambient!=null){ambient.release();ambient=null;}ambientReady=false;if(music!=null){music.release();music=null;}musicReady=false;pool.release();manager.abandonAudioFocusRequest(focus);}
}
