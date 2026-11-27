package org.reigncarnated.menu;

import android.os.Bundle;

public final class MissionTargetTest {
 public static void main(String[] args){
  Bundle launch=new Bundle();
  launch.putString("group","missions/sprawler");
  launch.putString("mission","S01");
  launch.putBoolean("campaign",true);
  launch.putString("slot","checkpointB");
  MissionActivity.Target loaded=MissionActivity.Target.from(launch);
  if(!loaded.slot.equals("checkpointB"))throw new AssertionError("Lost initial save slot");

  MissionActivity.Target advanced=new MissionActivity.Target(loaded.group,"S02",true,false,"");
  MissionActivity.Target recreated=MissionActivity.Target.from(advanced.toBundle());
  if(!recreated.group.equals(loaded.group)||!recreated.mission.equals("S02")||
     !recreated.campaign||recreated.individual||!recreated.slot.isEmpty())
   throw new AssertionError("Restored wrong mission or stale save slot");

  MissionActivity.Target saved=advanced.withSavedSlot("checkpointA").withSavedSlot("checkpointB");
  MissionActivity.Target afterSave=MissionActivity.Target.from(saved.toBundle());
  if(!afterSave.slot.equals("checkpointB")||!afterSave.group.equals(advanced.group)||
     !afterSave.mission.equals("S02")||!afterSave.campaign||afterSave.individual)
   throw new AssertionError("Activity recreation lost the most recent mid-mission save");

  launch.putBoolean("individualCampaign",true);
  MissionActivity.Target test=MissionActivity.Target.from(MissionActivity.Target.from(launch).toBundle());
  if(!test.individual)throw new AssertionError("Lost individual campaign isolation");
  System.out.println("PASS restored Sprawler handoff, save slot and individual mode");
 }
}
