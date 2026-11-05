package org.reigncarnated.menu;
public final class CampaignSelectionTest {
 static int checks;static void check(boolean v){++checks;if(!v)throw new AssertionError("Campaign selection check "+checks);}static void reject(String...rows){boolean bad=false;try{new CampaignSelection(rows);}catch(IllegalArgumentException e){bad=true;}check(bad);}
 public static void main(String[] args){
  CampaignSelection c=new CampaignSelection(new String[]{"missions/jda\tJ01\tmission\t1\t1","missions/jda\tJ06_intro\tcutscene\t1\t0","missions/sprawler\tS01_Intro\tcutscene\t0\t1","missions/sprawler\tS01\tmission\t1\t0","missions/sprawler\tOutro\tcutscene\t1\t0"});
  check(c.entries.size()==5&&c.starts.size()==2);check(c.starts.get(0).name.equals("J01")&&c.starts.get(1).name.equals("S01_Intro"));check(!c.starts.get(1).available);check(c.entries.get(1).cutscene&&c.entries.get(1).label().contains("Cutscene"));check(c.entries.get(2).label().contains("missing data"));check(c.entries.get(3).label().contains("Sprawler")&&c.entries.get(0).label().contains("JDA"));
  reject("missions/jda\tJ01\tmission\t1\t0");reject("missions/jda\tJ01\tmission\t1\t1","missions/jda\tJ02\tmission\t1\t1");reject("missions/jda\tJ01\tmission\t1\t1","missions/jda\tj01\tmission\t1\t0");reject("missions/../secret\tJ01\tmission\t1\t1");reject("missions/jda\t../J01\tmission\t1\t1");reject("missions/jda\tJ01\tunknown\t1\t1");reject("missions/jda\tJ01\tmission\t2\t1");reject("missions/jda\tJ01");check(new CampaignSelection(new String[0]).entries.isEmpty());
  boolean immutable=false;try{c.entries.clear();}catch(UnsupportedOperationException e){immutable=true;}check(immutable);
  System.out.println(checks+" campaign opening/individual/cutscene/missing/duplicate/path/immutable selection checks PASS");
 }
}
