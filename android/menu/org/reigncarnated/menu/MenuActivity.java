package org.reigncarnated.menu;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.opengl.GLSurfaceView;
import android.view.MotionEvent;
import android.view.KeyEvent;
import android.view.View;
import android.view.Gravity;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.text.InputType;
import android.widget.*;
import java.io.*;
import android.net.Uri;
import java.util.*;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public final class MenuActivity extends Activity {
 static {System.loadLibrary("reign_menu");}
 private static final int PICK_FOLDER=20,PICK_ZIP=21,PICK_MUSIC=22,SAVE_GAME_DATA=23,PLAY_MISSION=24;
 private File pendingGameExport;
 private native long openNative(String pack,String profiles,String shell,String version);
 private native String beginInstantNative(long h);
 private native String setInstantOptionsNative(long h,int map,int opponents,int difficulty,int seed);
 private native String returnMainNative(long h);
 private String apkVersion;private boolean instantOpening;private android.app.AlertDialog missionDialog;
 private Button instantButton,campaignButton;
 private native String[] catalogNative(long h,boolean campaign);
 private native String[] difficultyNative(long h);
 private native String errorNative();
 private native String debugNative(long h,int action);
 private native int[] captureNative(long h);
 private native String diagnosticsNative(long h);
 private native String performanceNative(long h);

 private native String drawNative(long h,int w,int height,long ms);
 private native String inputNative(long h,int kind,int x,int y,String text);
 private native String resetGpuNative(long h);
 private native void closeNative(long h);
 private native long[] checkpointNative(long h);
 private native String restoreNative(long h,long clip,long elapsed,long now);
 private volatile long[] launchCheckpoint;
 private volatile int audioGeneration;
 private native String[] audioNative(long h);
 private native String ambientNative(long h);
 private native int[] movieInfoNative(long h);
 private native short[] moviePcmNative(long h);
 private IntroAudioPlayer introAudio;private volatile boolean menuReady;
 private native float volumeNative(long h);
 private volatile MenuAudioPlayer audioPlayer;
 private MenuView view;private LinearLayout setup;private TextView status;private Button choose,chooseZip,cancelImport;
 private String report="",dataRoot;private RenderMetrics metrics;private volatile boolean importing,cancelled,destroyed;
 private volatile boolean resumed;
 private final StringBuilder history=new StringBuilder();
 private int dp(int n){return Math.round(n*getResources().getDisplayMetrics().density);}
 private boolean hasData(){String base=getPreferences(0).getString("pack",null),shell=getPreferences(0).getString("shell",null);return base!=null&&shell!=null&&new File(base).isFile()&&new File(shell).isFile()&&dataRoot!=null&&new File(dataRoot,"worlds/sprawl.zwp").isFile();}
 private void record(String s){Diagnostics.event(s);runOnUiThread(()->{if(destroyed)return;report=s;if(history.length()>12000)history.delete(0,6000);history.append(s).append('\n');});}
 private void message(String s){if(s==null)return;runOnUiThread(()->{
  if(destroyed)return;if(s.equals("@quit")){finish();return;}if(s.equals("@options")){showOptions();return;}if(s.equals("@instant")){showInstantAction();return;}if(s.equals("@instant-started")||s.equals("@instant-open")){setInstantOpening(resumed);return;}if(s.equals("@instant-returning")){setInstantOpening(resumed);((InputMethodManager)getSystemService(INPUT_METHOD_SERVICE)).hideSoftInputFromWindow(view.getWindowToken(),0);return;}if(s.equals("@main-open")){setInstantOpening(false);return;}if(s.equals("@keyboard")){if(resumed)showKeyboard();return;}if(s.startsWith("@instant-launch\t")){launchInstant(s);return;}if(s.equals("@campaign")){showMissionOptions(true);return;}
  record(s);if(setup.getVisibility()==View.VISIBLE)status.setText(s);else Toast.makeText(this,s,Toast.LENGTH_LONG).show();
 });}
 private void startupFailure(String s){runOnUiThread(()->{if(destroyed)return;setup.setVisibility(View.VISIBLE);message(s);});}
 @Override public void onCreate(Bundle saved){
  super.onCreate(saved);Diagnostics.init(this);try{android.content.pm.PackageInfo p=getPackageManager().getPackageInfo(getPackageName(),0);apkVersion="Reigncarnated "+p.versionName+" (APK "+p.versionCode+")";}catch(android.content.pm.PackageManager.NameNotFoundException e){throw new IllegalStateException(e);}dataRoot=getPreferences(0).getString("dataRoot",null);
  if(saved!=null){String name=saved.getString("gameDataExport");if(name!=null&&name.matches("game-data-[a-zA-Z0-9-]+\\.zip")){File file=new File(new File(getCacheDir(),"game-data-export"),name);if(file.isFile())pendingGameExport=file;}}
  FrameLayout screen=new FrameLayout(this);view=new MenuView();screen.addView(view,new FrameLayout.LayoutParams(-1,-1));
  Button reportButton=new Button(this);reportButton.setText("Report / data");reportButton.setOnClickListener(v->showDebugTools());
  FrameLayout.LayoutParams tools=new FrameLayout.LayoutParams(-2,-2,Gravity.TOP|Gravity.END);screen.addView(reportButton,tools);
  campaignButton=new Button(this);campaignButton.setText("Campaign missions");campaignButton.setOnClickListener(v->showMissionOptions(true));FrameLayout.LayoutParams cp=new FrameLayout.LayoutParams(-2,-2,Gravity.TOP|Gravity.CENTER_HORIZONTAL);screen.addView(campaignButton,cp);
  instantButton=new Button(this);instantButton.setText("Instant Action");instantButton.setOnClickListener(v->showInstantAction());screen.addView(instantButton,new FrameLayout.LayoutParams(-2,-2,Gravity.TOP|Gravity.START));
  TextView version=new TextView(this);version.setText(apkVersion);version.setTextColor(0xffffffff);version.setBackgroundColor(0xb0000000);version.setPadding(dp(6),dp(3),dp(6),dp(3));screen.addView(version,new FrameLayout.LayoutParams(-2,-2,Gravity.BOTTOM|Gravity.START));
  setup=new LinearLayout(this);setup.setOrientation(LinearLayout.VERTICAL);setup.setGravity(Gravity.CENTER);setup.setPadding(dp(28),dp(24),dp(28),dp(24));setup.setBackgroundColor(0xff101710);
  TextView title=new TextView(this);title.setText("REIGNCARNATED");title.setTextColor(0xffaadd99);title.setTextSize(28);title.setGravity(Gravity.CENTER);setup.addView(title);
  TextView description=new TextView(this);description.setText("Select ZIP files from your legally owned Dark Reign 2 installation.\n\nFor Instant Action, include the complete packs, missions, worlds, sides, mods and library folders. Music is imported automatically. Select multiple ZIPs together if needed. Data is copied once; later launches play the original introduction and open the native menu.");description.setTextColor(0xffeeeeee);description.setGravity(Gravity.CENTER);description.setPadding(0,dp(20),0,dp(20));setup.addView(description);
  chooseZip=new Button(this);chooseZip.setText("Select game data ZIP(s)");chooseZip.setOnClickListener(v->pickZip());setup.addView(chooseZip);
  choose=new Button(this);choose.setText("Select game data folder");choose.setOnClickListener(v->pickFolder());setup.addView(choose);
  cancelImport=new Button(this);cancelImport.setText("Cancel import");cancelImport.setVisibility(View.GONE);cancelImport.setOnClickListener(v->{cancelled=true;status.setText("Cancelling import…");});setup.addView(cancelImport);
  Button back=new Button(this);back.setText("Return to existing game");back.setVisibility(hasData()?View.VISIBLE:View.GONE);back.setOnClickListener(v->{if(!importing)setup.setVisibility(View.GONE);});setup.addView(back);
  status=new TextView(this);status.setTextColor(0xffdddddd);status.setGravity(Gravity.CENTER);status.setPadding(0,dp(16),0,0);setup.addView(status);
  screen.addView(setup,new FrameLayout.LayoutParams(-1,-1));setup.setVisibility(hasData()?View.GONE:View.VISIBLE);metrics=new RenderMetrics(this,screen,"Menu");setContentView(screen);
  // Delete only unreferenced importer-owned snapshots, captured before a new import can start.
  final String keepRoot=dataRoot,keepMusicRoot=getPreferences(0).getString("audio.musicRoot",null);File parent=new File(getFilesDir(),"installations");File[] abandoned=parent.listFiles();
  if(abandoned!=null)new Thread(()->{for(File f:abandoned)if((f.getName().startsWith("import-")||f.getName().startsWith("data-"))&&!f.getAbsolutePath().equals(keepRoot)&&!f.getAbsolutePath().equals(keepMusicRoot))DataFolderImporter.remove(f);},"ReignCleanup").start();
 }
 private void showInstantAction(){
  if(!resumed)return;
  if(!hasData()){message("Import your complete game data first.");return;}if(instantOpening||(missionDialog!=null&&missionDialog.isShowing()))return;setInstantOpening(true);
  view.queueEvent(()->{String error=view.handle==0?"Wait for the main menu to load":beginInstantNative(view.handle);if(error!=null)runOnUiThread(()->{setInstantOpening(false);message(error);});});
 }
 private void setInstantOpening(boolean active){instantOpening=active;if(instantButton!=null)instantButton.setVisibility(active?View.GONE:View.VISIBLE);if(campaignButton!=null)campaignButton.setVisibility(active?View.GONE:View.VISIBLE);if(!active&&view!=null)((InputMethodManager)getSystemService(INPUT_METHOD_SERVICE)).hideSoftInputFromWindow(view.getWindowToken(),0);}
 private void returnToMain(){((InputMethodManager)getSystemService(INPUT_METHOD_SERVICE)).hideSoftInputFromWindow(view.getWindowToken(),0);view.queueEvent(()->{if(view.handle!=0){String error=returnMainNative(view.handle);if(error!=null)runOnUiThread(()->{setInstantOpening(false);message(error);});}else runOnUiThread(()->setInstantOpening(false));});}
 @Override public void onBackPressed(){if(instantOpening){returnToMain();return;}super.onBackPressed();}
 private void launchInstant(String action){
  if(!resumed)return;
  try{String[] p=action.split("\t",-1);if(p.length!=8)throw new IllegalArgumentException("Invalid native setup selection");int opponents=Integer.parseInt(p[3]),seed=Integer.parseInt(p[4]),map=Integer.parseInt(p[6]),difficulty=Integer.parseInt(p[7]);if(opponents<0||opponents>1||seed<=0||map<0||difficulty<0)throw new IllegalArgumentException("Invalid native setup options");
   getPreferences(0).edit().putInt("instantMap",map).putInt("opponents",opponents).putInt("seed",seed).putInt("instantDifficulty",difficulty).apply();
   Intent launch=new Intent(this,MissionActivity.class);launch.putExtra("installation",dataRoot);launch.putExtra("group",p[1]);launch.putExtra("mission",p[2]);launch.putExtra("campaign",false);launch.putExtra("opponents",opponents);launch.putExtra("seed",seed);launch.putExtra("difficulty",p[5]);launch.putExtra("musicRoot",selectedMusicPath());launch.putExtra("musicVolume",AndroidOptions.music(this)/100f);
   ((InputMethodManager)getSystemService(INPUT_METHOD_SERVICE)).hideSoftInputFromWindow(view.getWindowToken(),0);Diagnostics.event("Native setup launch "+p[1]+"/"+p[2]+" opponents="+opponents+" seed="+seed+" difficulty="+p[5]);AndroidOptions.exportTo(this,launch);startActivityForResult(launch,PLAY_MISSION);
  }catch(Exception e){message("Cannot launch Instant Action: "+e.getMessage());returnToMain();}
 }
 private void showMissionOptions(boolean campaign){
  if(!resumed)return;final int generation=audioGeneration;
  if(missionDialog!=null&&missionDialog.isShowing())return;if(campaign&&instantOpening)return;
  if(!hasData()){message("Import your complete game data first.");return;}
  view.queueEvent(()->{if(view.handle==0){message("Wait for the main menu to finish loading");return;}String[] rows=catalogNative(view.handle,campaign),difficulty=difficultyNative(view.handle);if(rows==null||difficulty==null){message(errorNative());if(!campaign)runOnUiThread(this::returnToMain);return;}runOnUiThread(()->{if(!destroyed&&resumed&&generation==audioGeneration)missionOptions(campaign,rows,difficulty);});});
 }
 private void missionOptions(boolean campaign,String[] rows,String[] difficulties){
  if(destroyed)return;if(rows.length==0){if(!campaign)returnToMain();message("No imported "+(campaign?"campaign":"Instant Action")+" missions were found. Import the complete missions folder.");return;}
  LinearLayout panel=new LinearLayout(this);panel.setOrientation(LinearLayout.VERTICAL);panel.setPadding(dp(16),dp(8),dp(16),dp(8));
  TextView mapLabel=new TextView(this);mapLabel.setText(campaign?"Campaign / mission":"Map");panel.addView(mapLabel);Spinner maps=new Spinner(this);String[] labels=new String[rows.length];for(int i=0;i<rows.length;i++)labels[i]=rows[i].replace('\t','/');maps.setAdapter(new ArrayAdapter<>(this,android.R.layout.simple_spinner_dropdown_item,labels));panel.addView(maps);
  int previous=getPreferences(0).getInt(campaign?"campaignMap":"instantMap",0);maps.setSelection(Math.min(previous,rows.length-1));
  TextView aiLabel=new TextView(this);aiLabel.setText("Opponent");Spinner ai=new Spinner(this);ai.setAdapter(new ArrayAdapter<>(this,android.R.layout.simple_spinner_dropdown_item,new String[]{"No opponent (practice)","One original AI opponent"}));ai.setSelection(getPreferences(0).getInt("opponents",0));if(!campaign){panel.addView(aiLabel);panel.addView(ai);}
  TextView levelLabel=new TextView(this);levelLabel.setText("Difficulty");panel.addView(levelLabel);Spinner level=new Spinner(this);String[] levelLabels=difficulties.clone();if(levelLabels.length>0)levelLabels[0]="Mission default";level.setAdapter(new ArrayAdapter<>(this,android.R.layout.simple_spinner_dropdown_item,levelLabels));panel.addView(level);
  TextView seedLabel=new TextView(this);seedLabel.setText("Random seed");if(!campaign)panel.addView(seedLabel);EditText seed=new EditText(this);seed.setInputType(android.text.InputType.TYPE_CLASS_NUMBER);seed.setText(Integer.toString(getPreferences(0).getInt("seed",1)));if(!campaign)panel.addView(seed);
  android.app.AlertDialog dialog=new android.app.AlertDialog.Builder(this).setTitle(campaign?"Campaign missions":"Instant Action options").setView(panel).setPositiveButton("Launch",null).setNegativeButton("Cancel",null).create();
  missionDialog=dialog;dialog.setOnDismissListener(d->{missionDialog=null;if(!campaign)returnToMain();});
  dialog.setOnShowListener(d->dialog.getButton(android.app.AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{try{int value=campaign?2147483647:Integer.parseInt(seed.getText().toString());if(value<=0)throw new NumberFormatException();int index=maps.getSelectedItemPosition();String[] parts=rows[index].split("\t",-1);if(parts.length!=2)throw new IllegalStateException("Invalid mission entry");android.content.SharedPreferences.Editor saved=getPreferences(0).edit().putInt(campaign?"campaignMap":"instantMap",index);if(!campaign)saved.putInt("opponents",ai.getSelectedItemPosition()).putInt("seed",value);saved.apply();
   Intent launch=new Intent(this,MissionActivity.class);launch.putExtra("installation",dataRoot);launch.putExtra("group",parts[0]);launch.putExtra("mission",parts[1]);launch.putExtra("campaign",campaign);launch.putExtra("opponents",campaign?0:ai.getSelectedItemPosition());launch.putExtra("seed",value);launch.putExtra("difficulty",difficulties[level.getSelectedItemPosition()]);launch.putExtra("musicRoot",selectedMusicPath());launch.putExtra("musicVolume",AndroidOptions.music(this)/100f);Diagnostics.event("Launch "+parts[0]+"/"+parts[1]+" campaign="+campaign+" opponents="+(campaign?0:ai.getSelectedItemPosition())+" seed="+value);dialog.dismiss();AndroidOptions.exportTo(this,launch);startActivityForResult(launch,PLAY_MISSION);
  }catch(NumberFormatException e){seed.setError("Enter a positive seed up to 2147483647");}catch(Exception e){message(e.toString());}}));dialog.show();
 }
 private void showDebugTools(){new android.app.AlertDialog.Builder(this).setTitle("Recovery debug tools").setItems(new String[]{"Share full report","GPU pixel checks","Fast stream (normal)","Synchronized stream (compare)","Audit next frame","Capture frame and report","Replay intro","Import music ZIP","Import game ZIP","Collect diagnostics ZIP (all in one)","Export complete game-data ZIP"},(d,which)->{
  if(which==10){exportGameData();return;}
  if(which==9){collectDiagnostics();return;}
  if(which==7){if(!importing){Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*").addCategory(Intent.CATEGORY_OPENABLE);startActivityForResult(i,PICK_MUSIC);}return;}
  if(which==8){pickZip();return;}
  if(which==2||which==3)getSharedPreferences("debug",0).edit().putBoolean("synchronized",which==3).apply();
  view.queueEvent(()->{if(view.handle==0){message("Open game data first");return;}if(which==0||which==5){Diagnostics.share(this,metrics.latest()+"\n"+diagnosticsNative(view.handle),which==5?captureNative(view.handle):null);}
   else if(which==6){message(restoreNative(view.handle,1,0,android.os.SystemClock.uptimeMillis()));runOnUiThread(()->{menuReady=false;if(audioPlayer!=null){audioPlayer.close();audioPlayer=null;}});}
   else message(debugNative(view.handle,which==4?4:which));});
 }).show();}
 private void importMusic(android.net.Uri uri){if(uri==null||importing)return;importing=true;cancelled=false;Diagnostics.event("Supplemental music import started");
  new Thread(()->{File cache=new File(getCacheDir(),"music-import-"+UUID.randomUUID()+".zip");DataFolderImporter.Result imported=null;
   try{try(InputStream in=getContentResolver().openInputStream(uri);FileOutputStream out=new FileOutputStream(cache)){if(in==null)throw new IOException("Cannot open music ZIP");byte[] b=new byte[262144];long size=0;int n;while((n=in.read(b))!=-1){if(cancelled||destroyed)throw new IOException("Import cancelled");if((size+=n)>DataFolderImporter.MAX_TOTAL)throw new IOException("Music ZIP exceeds 2 GiB");out.write(b,0,n);}}
    try(ZipGameSource source=new ZipGameSource(Collections.singletonList(cache),false)){imported=new DataFolderImporter(source,()->cancelled||destroyed,this::record).runMusic(source.root(),new File(getFilesDir(),"music-imports"));}
    final DataFolderImporter.Result ready=imported;runOnUiThread(()->{importing=false;if(destroyed||cancelled){DataFolderImporter.remove(ready.directory);return;}String previousMusic=getPreferences(0).getString("audio.musicRoot",null);if(!getPreferences(0).edit().putString("audio.musicRoot",ready.directory.getAbsolutePath()).commit()){getPreferences(0).edit().putString("audio.musicRoot",previousMusic).commit();DataFolderImporter.remove(ready.directory);message("Cannot save music selection");return;}record("Music imported: "+ready.tracks);startAudio();});
   }catch(Exception e){if(imported!=null)DataFolderImporter.remove(imported.directory);runOnUiThread(()->{importing=false;message("Music import failed: "+e.getMessage());});}finally{cache.delete();}
  },"ReignMusicImport").start();
 }
 private void exportGameData(){
  if(importing)return;if(!hasData()){message("Import your game data first.");return;}importing=true;cancelled=false;
  final File installation=new File(dataRoot);String selected=selectedMusicPath();final File musicRoot=selected==null?null:new File(selected);
  android.app.ProgressDialog progress=new android.app.ProgressDialog(this);progress.setTitle("Creating one game-data ZIP");progress.setMessage("Checking Sprawl, music and runtime files…");progress.setCancelable(false);progress.setButton(android.app.ProgressDialog.BUTTON_NEGATIVE,"Cancel",(d,w)->cancelled=true);progress.show();
  new Thread(()->{
   File output=new File(new File(getCacheDir(),"game-data-export"),"game-data-"+UUID.randomUUID()+".zip");
   try{
    File parent=output.getParentFile();if(parent.isDirectory()){File[] old=parent.listFiles();if(old!=null)for(File file:old)if(file.getName().startsWith("game-data-")&&(pendingGameExport==null||!file.equals(pendingGameExport)))file.delete();}
    String receipt;try(InputStream in=getAssets().open("build/source.json");ByteArrayOutputStream bytes=new ByteArrayOutputStream()){byte[] block=new byte[4096];int n;while((n=in.read(block))!=-1){bytes.write(block,0,n);if(bytes.size()>65536)throw new IOException("APK receipt exceeds export limit");}receipt=bytes.toString("UTF-8");}
    GameDataZip.Result result=GameDataZip.write(installation,musicRoot,output,receipt,()->cancelled||destroyed,text->runOnUiThread(()->{if(!destroyed&&progress.isShowing())progress.setMessage(text);}));
    runOnUiThread(()->{importing=false;if(progress.isShowing())progress.dismiss();if(destroyed||cancelled){result.file.delete();return;}pendingGameExport=result.file;record("Game-data ZIP ready: "+result.files+" files, "+result.tracks+" music tracks; SHA256 "+result.sha256);showGameDataReady(result.tracks);});
   }catch(Exception e){runOnUiThread(()->{importing=false;if(progress.isShowing())progress.dismiss();if(!destroyed)message("Export failed: "+e.getMessage());});}
  },"ReignGameDataExport").start();
 }
 private void showGameDataReady(int tracks){
  new android.app.AlertDialog.Builder(this).setTitle("Complete game-data ZIP ready").setMessage("Includes Sprawl, all currently imported runtime folders and "+tracks+" music tracks. Save it, then import this one ZIP on your device. Profiles and saves are excluded.").setPositiveButton("Save ZIP",(d,w)->startActivityForResult(new Intent(Intent.ACTION_CREATE_DOCUMENT).setType("application/zip").addCategory(Intent.CATEGORY_OPENABLE).putExtra(Intent.EXTRA_TITLE,"Reigncarnated-Complete-Game-Data.zip"),SAVE_GAME_DATA)).setNegativeButton("Close",null).show();
 }
 private void saveGameData(int result,Intent data){
  if(result!=RESULT_OK||data==null||data.getData()==null)return;final File source=pendingGameExport;
  if(source==null||!source.isFile()){message("Export file is unavailable. Create the game-data ZIP again.");return;}importing=true;final Uri destination=data.getData();
  new Thread(()->{try(InputStream in=new FileInputStream(source);OutputStream out=getContentResolver().openOutputStream(destination,"w")){
   if(out==null)throw new IOException("Cannot open the selected destination");byte[] block=new byte[262144];long count=0;int n;while((n=in.read(block))!=-1){out.write(block,0,n);count+=n;}if(count!=source.length())throw new IOException("Incomplete export copy");out.flush();
  }catch(Exception e){runOnUiThread(()->{importing=false;message("Cannot save game-data ZIP: "+e.getMessage());});return;}
   runOnUiThread(()->{importing=false;message("Saved Reigncarnated-Complete-Game-Data.zip. Select that file alone for future game-data imports.");if(source.equals(pendingGameExport))pendingGameExport=null;source.delete();});
  },"ReignSaveGameData").start();
 }
 private void showKeyboard(){view.requestFocus();((InputMethodManager)getSystemService(INPUT_METHOD_SERVICE)).showSoftInput(view,InputMethodManager.SHOW_IMPLICIT);}
 private void pickZip(){if(importing)return;Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT);i.addCategory(Intent.CATEGORY_OPENABLE);i.setType("*/*");i.putExtra(Intent.EXTRA_ALLOW_MULTIPLE,true);i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);startActivityForResult(i,PICK_ZIP);}
 private void pickFolder(){if(importing)return;Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);startActivityForResult(i,PICK_FOLDER);}
 private String selectedMusicPath(){String preferred=getPreferences(0).getString("audio.musicRoot",null);File selected=GameDataZip.musicRoot(dataRoot==null?null:new File(dataRoot),preferred==null?null:new File(preferred));return selected==null?null:selected.getAbsolutePath();}
 private List<String> musicTracks(){String musicRoot=selectedMusicPath();if(musicRoot!=null)return DataFolderImporter.playlist(new File(musicRoot));String legacy=getPreferences(0).getString("music",null);return legacy!=null&&new File(legacy).isFile()?Collections.singletonList(legacy):Collections.emptyList();}
 private void startAudio(){if(!resumed)return;if(audioPlayer!=null)audioPlayer.close();audioPlayer=new MenuAudioPlayer(this,this::message);audioPlayer.setMusicVolume(AndroidOptions.music(this)/100f);if(menuReady)audioPlayer.playPlaylist(musicTracks());}
 @Override protected void onResume(){super.onResume();Diagnostics.event("Menu resume");Diagnostics.captureExits(this);metrics.reset();AndroidOptions.applyWindow(this,metrics);resumed=true;menuReady=false;startAudio();view.onResume();view.requestRender();}
 @Override protected void onPause(){if(optionsDialog!=null)optionsDialog.dismiss();Diagnostics.event("Menu pause");if(missionDialog!=null)missionDialog.dismiss();setInstantOpening(false);resumed=false;audioGeneration++;menuReady=false;if(introAudio!=null){introAudio.close();introAudio=null;}MenuAudioPlayer old=audioPlayer;audioPlayer=null;if(old!=null)old.close();view.queueEvent(()->{if(view.sweep!=null)view.sweep.cancel();if(view.handle!=0){launchCheckpoint=checkpointNative(view.handle);closeNative(view.handle);view.handle=0;}});view.onPause();super.onPause();}
 @Override protected void onSaveInstanceState(Bundle out){if(pendingGameExport!=null)out.putString("gameDataExport",pendingGameExport.getName());super.onSaveInstanceState(out);}
 @Override protected void onDestroy(){destroyed=true;cancelled=true;super.onDestroy();}
 private void collectDiagnostics(){
  android.widget.Toast.makeText(this,"Collecting fast/synchronized frames and GPU checks. Keep this screen open until the ZIP share dialog appears.",android.widget.Toast.LENGTH_LONG).show();
  final boolean original=getSharedPreferences("debug",0).getBoolean("synchronized",false);
  view.queueEvent(()->{if(view.sweep!=null&&!view.sweep.done())return;if(view.handle==0){Diagnostics.event("Diagnostic collection unavailable: no native handle");Diagnostics.shareZip(this,java.util.Collections.singletonMap("collection.txt","No native handle\n"+metrics.latest()),new java.util.LinkedHashMap<>());return;}
   view.sweep=new DiagnosticSweep(new DiagnosticSweep.Backend(){public String debug(int action){return debugNative(view.handle,action);}public String report(){return metrics.latest()+"\n"+diagnosticsNative(view.handle);}public int[] capture(){return captureNative(view.handle);}},(reports,pictures)->Diagnostics.shareZip(MenuActivity.this,reports,pictures),original);
   view.sweep.start();if(view.failed)view.sweep.abort("Render loop previously failed; exporting available diagnostics");
  });
 }
 private final class MenuView extends GLSurfaceView implements GLSurfaceView.Renderer {
  DiagnosticSweep sweep;
  long handle;int width,height;String pack,shell;boolean failed;String gpu="";
  MenuView(){super(MenuActivity.this);pack=getPreferences(0).getString("pack",null);shell=getPreferences(0).getString("shell",null);if(!hasData())pack=null;setFocusable(true);setFocusableInTouchMode(true);setEGLContextClientVersion(3);setEGLConfigChooser(8,8,8,8,16,0);setRenderer(this);setRenderMode(RENDERMODE_CONTINUOUSLY);}
  public void onSurfaceCreated(GL10 gl,EGLConfig c){if(sweep!=null){sweep.cancel();sweep=null;}runOnUiThread(()->{setInstantOpening(false);if(missionDialog!=null)missionDialog.dismiss();});gpu="Device: "+android.os.Build.MANUFACTURER+" "+android.os.Build.MODEL+" API "+android.os.Build.VERSION.SDK_INT+"\nGPU: "+gl.glGetString(GL10.GL_RENDERER);failed=false;if(handle!=0){String error=resetGpuNative(handle);if(error!=null){failed=true;message(error);}else debugNative(handle,getSharedPreferences("debug",0).getBoolean("synchronized",false)?3:2);}}
  public void onSurfaceChanged(GL10 gl,int w,int h){width=w;height=h;requestRender();}
  public void onDrawFrame(GL10 gl){final int generation=audioGeneration;if(!resumed)return;if(width<=0||height<=0||pack==null||failed||importing)return;if(handle==0){handle=openNative(pack,new File(getFilesDir(),"profiles").getAbsolutePath(),shell,apkVersion);if(handle==0){failed=true;startupFailure("Native startup failed: "+errorNative());return;}String optionsError=setInstantOptionsNative(handle,getPreferences(0).getInt("instantMap",0),getPreferences(0).getInt("opponents",0),getPreferences(0).getInt("instantDifficulty",0),getPreferences(0).getInt("seed",1));if(optionsError!=null){failed=true;startupFailure(optionsError);return;}inputNative(handle,5,Math.round(AndroidOptions.effects(MenuActivity.this)*100),0,null);long[] checkpoint=launchCheckpoint;if(checkpoint!=null){String restoreError=restoreNative(handle,checkpoint[0],checkpoint[1],android.os.SystemClock.uptimeMillis());if(restoreError!=null){failed=true;startupFailure(restoreError);return;}launchCheckpoint=null;}debugNative(handle,getSharedPreferences("debug",0).getBoolean("synchronized",false)?3:2);record(gpu+"\nOriginal intro sequence loaded. Native shell follows the movies; tap to skip the current clip.");}long drawStart=System.nanoTime();String e=drawNative(handle,width,height,android.os.SystemClock.uptimeMillis());long drawEnd=System.nanoTime();metrics.frame(drawStart,drawEnd,()->performanceNative(handle));if(sweep!=null){if(e!=null&&!e.startsWith("@"))sweep.abort(e);else sweep.frame(drawStart,drawEnd);}String ambientPath=ambientNative(handle);int[] movie=movieInfoNative(handle);short[] pcm=moviePcmNative(handle);
   if((movie!=null&&movie[0]>0)||!menuReady)runOnUiThread(()->{if(destroyed||!resumed||generation!=audioGeneration)return;try{if(movie!=null&&movie[0]>0){if(movie[1]>0){if(introAudio==null||introAudio.movie!=movie[0]){if(introAudio!=null)introAudio.close();introAudio=new IntroAudioPlayer(MenuActivity.this,movie[0],movie[1],MenuActivity.this::message);}introAudio.accept(pcm);}}else if(!menuReady){if(introAudio!=null){introAudio.close();introAudio=null;}menuReady=true;startAudio();if(audioPlayer!=null)audioPlayer.playAmbient(ambientPath);}}catch(Exception ex){message("Intro audio failed: "+ex.getMessage());}});
   String[] sounds=audioNative(handle);MenuAudioPlayer player=audioPlayer;if(player!=null&&sounds!=null&&sounds.length>0)runOnUiThread(()->{if(audioPlayer==player)player.accept(sounds);});if(e!=null){if(e.startsWith("@"))message(e);else{failed=true;startupFailure("Render failure: "+e);}}}
  void input(int kind,int x,int y,String text){queueEvent(()->{if(handle!=0)message(inputNative(handle,kind,x,y,text));});requestRender();}
  @Override public boolean onTouchEvent(MotionEvent e){requestFocus();if(e.getPointerCount()!=1){input(2,-32768,-32768,null);return true;}float scale=Math.min(width/640f,height/480f);if(scale<=0)return true;int x=(int)((e.getX()-(width-640*scale)/2)/scale),y=(int)((e.getY()-(height-480*scale)/2)/scale);int a=e.getActionMasked();if(a==MotionEvent.ACTION_DOWN)input(1,x,y,null);else if(a==MotionEvent.ACTION_UP)input(2,x,y,null);else if(a==MotionEvent.ACTION_CANCEL)input(2,-32768,-32768,null);else if(a==MotionEvent.ACTION_MOVE)input(0,x,y,null);return true;}
  @Override public boolean onCheckIsTextEditor(){return true;}
  @Override public InputConnection onCreateInputConnection(EditorInfo info){info.inputType=instantOpening?InputType.TYPE_CLASS_NUMBER:InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;info.imeOptions=EditorInfo.IME_ACTION_DONE|EditorInfo.IME_FLAG_NO_EXTRACT_UI;return new BaseInputConnection(this,false){@Override public boolean commitText(CharSequence text,int p){input(3,0,0,text.toString());return true;}@Override public boolean deleteSurroundingText(int before,int after){if(before<0||after<0||before>256||after>256)return false;for(int i=0;i<before;i++)input(4,0x0e,0,null);for(int i=0;i<after;i++)input(4,0xd3,0,null);return true;}@Override public boolean performEditorAction(int action){input(4,0x1c,0,null);return true;}@Override public boolean sendKeyEvent(KeyEvent e){if(e.getAction()!=KeyEvent.ACTION_DOWN)return true;if(e.getKeyCode()==KeyEvent.KEYCODE_DEL)input(4,0x0e,0,null);else if(e.getKeyCode()==KeyEvent.KEYCODE_ENTER)input(4,0x1c,0,null);else{int c=e.getUnicodeChar();if(c>=32&&c<=126)input(3,0,0,String.valueOf((char)c));}return true;}};}
 }
 private android.app.AlertDialog optionsDialog;
 private void showOptions(){if(!resumed||optionsDialog!=null)return;Diagnostics.event("Android options opened from main menu");optionsDialog=AndroidOptions.show(this,()->{AndroidOptions.applyWindow(this,metrics);if(audioPlayer!=null)audioPlayer.setMusicVolume(AndroidOptions.music(this)/100f);view.queueEvent(()->{if(view.handle!=0)message(inputNative(view.handle,5,Math.round(AndroidOptions.effects(this)*100),0,null));});});optionsDialog.setOnDismissListener(d->optionsDialog=null);}
 @Override protected void onActivityResult(int request,int result,Intent data){
  super.onActivityResult(request,result,data);if(request==PLAY_MISSION){if(result==RESULT_OK)AndroidOptions.importFrom(this,data);return;}if(request==SAVE_GAME_DATA){saveGameData(result,data);return;}if(Diagnostics.saveResult(this,request,result,data))return;if(request==PICK_MUSIC){if(result==RESULT_OK&&data!=null)importMusic(data.getData());return;}if((request!=PICK_FOLDER&&request!=PICK_ZIP)||result!=RESULT_OK||data==null||importing)return;
  final List<Uri> uris=new ArrayList<>();if(data.getClipData()!=null){for(int i=0;i<data.getClipData().getItemCount();i++)uris.add(data.getClipData().getItemAt(i).getUri());}else if(data.getData()!=null)uris.add(data.getData());if(uris.isEmpty())return;
  importing=true;cancelled=false;setup.setVisibility(View.VISIBLE);choose.setEnabled(false);chooseZip.setEnabled(false);cancelImport.setVisibility(View.VISIBLE);status.setText("Checking game data…");
  new Thread(()->{
   File cache=new File(getCacheDir(),"zip-"+UUID.randomUUID());
   try{
    DataFolderImporter.Result imported;
    if(request==PICK_FOLDER){DocumentTreeSource source=new DocumentTreeSource(getContentResolver(),uris.get(0));imported=new DataFolderImporter(source,()->cancelled||destroyed,this::message).run(source.root(),new File(getFilesDir(),"installations"));}
    else {
     if(uris.size()>16)throw new IOException("Select at most 16 ZIP files");if(!cache.mkdir())throw new IOException("Cannot create ZIP cache");List<File> files=new ArrayList<>();long total=0,reported=0;
     for(Uri uri:uris){File file=new File(cache,files.size()+".zip");message("Reading ZIP "+(files.size()+1)+" of "+uris.size()+"…");
      try(InputStream in=getContentResolver().openInputStream(uri);FileOutputStream out=new FileOutputStream(file)){
       if(in==null)throw new IOException("Cannot open selected ZIP");byte[] buffer=new byte[262144];int n;
       while((n=in.read(buffer))!=-1){if(cancelled||destroyed)throw new IOException("Import cancelled");total+=n;if(total>DataFolderImporter.MAX_TOTAL)throw new IOException("ZIP input exceeds 2 GiB");out.write(buffer,0,n);if(total-reported>=32L*1024*1024){reported=total;message("Reading ZIP data: "+total/1024/1024+" MiB");}}
      }files.add(file);
     }
     try(ZipGameSource source=new ZipGameSource(files)){imported=new DataFolderImporter(source,()->cancelled||destroyed,this::message).run(source.root(),new File(getFilesDir(),"installations"));}
    }
    if(!new File(imported.directory,"worlds/sprawl.zwp").isFile()){DataFolderImporter.remove(imported.directory);throw new IOException("Missing worlds/sprawl.zwp, required for the live 3D menu");}
    runOnUiThread(()->finishImport(imported));
   }catch(Exception e){runOnUiThread(()->{if(destroyed)return;importing=false;choose.setEnabled(true);chooseZip.setEnabled(true);cancelImport.setVisibility(View.GONE);message("Import failed: "+e.getMessage()+"\nYour previous game data is unchanged.");});}
   finally{DataFolderImporter.remove(cache);}
  },"ReignDataImport").start();
 }
 private void finishImport(DataFolderImporter.Result imported){
  if(destroyed||cancelled){new Thread(()->DataFolderImporter.remove(imported.directory),"ReignCleanup").start();importing=false;if(!destroyed){choose.setEnabled(true);chooseZip.setEnabled(true);cancelImport.setVisibility(View.GONE);message("Import cancelled. Previous data kept.");}return;}
  final String oldPack=getPreferences(0).getString("pack",null),oldShell=getPreferences(0).getString("shell",null),oldMusic=getPreferences(0).getString("music",null);
  final String previousAudioRoot=getPreferences(0).getString("audio.musicRoot",null),keepAudioRoot=selectedMusicPath();
  String path=imported.directory.getAbsolutePath(),base=new File(imported.directory,"base.zwp").getAbsolutePath(),shell=new File(imported.directory,"shell.zwp").getAbsolutePath();
  String newAudioRoot=imported.tracks>0?path:keepAudioRoot;
  if(!getPreferences(0).edit().putString("audio.musicRoot",newAudioRoot).putString("dataRoot",path).putString("pack",base).putString("shell",shell).remove("music").commit()){
   getPreferences(0).edit().putString("audio.musicRoot",previousAudioRoot).putString("dataRoot",dataRoot).putString("pack",oldPack).putString("shell",oldShell).putString("music",oldMusic).commit();
   new Thread(()->DataFolderImporter.remove(imported.directory),"ReignCleanup").start();importing=false;choose.setEnabled(true);chooseZip.setEnabled(true);cancelImport.setVisibility(View.GONE);message("Could not save setup. Please try again.");return;
  }
  final String previous=dataRoot;dataRoot=path;importing=false;choose.setEnabled(true);chooseZip.setEnabled(true);cancelImport.setVisibility(View.GONE);setup.setVisibility(View.GONE);
  view.queueEvent(()->{if(view.handle!=0){closeNative(view.handle);view.handle=0;}launchCheckpoint=null;audioGeneration++;view.pack=base;view.shell=shell;view.failed=false;
   if(previous!=null&&!previous.equals(path)&&!previous.equals(newAudioRoot)){File old=new File(previous);if(old.getName().startsWith("data-")&&old.getParentFile().equals(new File(getFilesDir(),"installations")))new Thread(()->DataFolderImporter.remove(old),"ReignCleanup").start();}
  });view.requestRender();startAudio();
  record("Native shell resource packs imported: "+imported.scenePacks+"/3. Live scene rendering enabled.");
  message(imported.tracks==0?(newAudioRoot==null?"Game data ready. No music tracks found; menu sound effects remain available.":"Game data ready. Existing music retained."):"Game data ready. "+imported.tracks+" music tracks imported.");
 }
}

