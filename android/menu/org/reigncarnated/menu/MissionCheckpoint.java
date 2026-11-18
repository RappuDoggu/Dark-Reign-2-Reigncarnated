package org.reigncarnated.menu;

import android.content.Intent;
import android.util.AtomicFile;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import org.json.JSONException;
import org.json.JSONObject;

/** One published snapshot. The native writer always targets the other slot first. */
final class MissionCheckpoint {
 private final AtomicFile file;
 MissionCheckpoint(File root){file=new AtomicFile(new File(root,"mission-checkpoint.json"));}

 JSONObject read() throws IOException, JSONException {
  byte[] bytes;
  try {
   bytes=file.readFully();
  } catch(FileNotFoundException e) {
   File base=file.getBaseFile();
   if(base.exists()||new File(base.getPath()+".bak").exists())throw e;
   return null;
  }
  JSONObject state=new JSONObject(new String(bytes,StandardCharsets.UTF_8));
  String slot=state.getString("slot");
  if(!slot.equals("checkpointA")&&!slot.equals("checkpointB"))throw new IOException("Unknown mission save slot");
  return state;
 }

 String nextSlot() throws IOException, JSONException {
  JSONObject previous=read();
  return previous!=null&&previous.getString("slot").equals("checkpointA")?"checkpointB":"checkpointA";
 }

 void publish(String slot,String group,String mission,boolean campaign,Intent launch) throws IOException, JSONException {
  JSONObject state=new JSONObject().put("slot",slot).put("group",group).put("mission",mission)
   .put("campaign",campaign).put("opponents",launch.getIntExtra("opponents",0))
   .put("seed",launch.getIntExtra("seed",1)).put("difficulty",launch.getStringExtra("difficulty"))
   .put("startingResources",launch.getIntExtra("startingResources",-1));
  byte[] bytes=state.toString().getBytes(StandardCharsets.UTF_8);
  FileOutputStream out=file.startWrite();
  try{out.write(bytes);file.finishWrite(out);}catch(IOException e){file.failWrite(out);throw e;}
 }

 static void restore(Intent launch,JSONObject state)throws JSONException {
  for(String key:new String[]{"slot","group","mission"})launch.putExtra(key,state.getString(key));
  launch.putExtra("difficulty",state.optString("difficulty",""));
  launch.putExtra("campaign",state.getBoolean("campaign"));
  for(String key:new String[]{"seed","opponents","startingResources"})launch.putExtra(key,state.getInt(key));
 }
}
