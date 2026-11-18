package org.reigncarnated.menu;
import java.util.*;
import java.io.*;
import java.nio.file.*;
/** Ordered launch data shared by the campaign hub and individual test picker. */
final class CampaignSelection {
 static final class Entry {
  final String group,name;final boolean cutscene,available,first;
  Entry(String[] p){group=p[0];name=p[1];cutscene=p[2].equals("cutscene");available=p[3].equals("1");first=p[4].equals("1");}
  String faction(){String s=group.substring(9);return s.equalsIgnoreCase("jda")?"JDA":s.equalsIgnoreCase("sprawler")?"Sprawler":s;}
  String label(){return faction()+" — "+(cutscene?"Cutscene: ":"Mission: ")+name+(available?"":" (missing data)");}
 }
 final List<Entry> entries;final List<Entry> starts;
 CampaignSelection(String[] rows){
  List<Entry> all=new ArrayList<>(),firsts=new ArrayList<>();Set<String> groups=new HashSet<>(),seen=new HashSet<>();
  for(String row:rows){String[] p=row.split("\t",-1);if(p.length!=5||!p[0].startsWith("missions/")||!simple(p[0].substring(9))||!simple(p[1])||(!p[2].equals("mission")&&!p[2].equals("cutscene"))||!bit(p[3])||!bit(p[4]))throw new IllegalArgumentException("Invalid campaign entry");
   Entry e=new Entry(p);String group=e.group.toLowerCase(Locale.ROOT);if(!seen.add(group+"/"+e.name.toLowerCase(Locale.ROOT)))throw new IllegalArgumentException("Duplicate campaign entry");
   boolean newGroup=groups.add(group);if(newGroup!=e.first)throw new IllegalArgumentException("Campaign opening is not the first declared entry");
   if(e.first)firsts.add(e);all.add(e);
  }
  entries=Collections.unmodifiableList(all);starts=Collections.unmodifiableList(firsts);
 }
 static String groupKey(String group){
  return group.replace((char)92,'/').toLowerCase(Locale.ROOT);
 }
 Entry checkpoint(String group,String mission){
  for(Entry entry:entries){
   if(groupKey(entry.group).equals(groupKey(group))&&entry.name.equalsIgnoreCase(mission)&&entry.available)return entry;
  }
  return null;
 }
 private static boolean bit(String s){return s.equals("0")||s.equals("1");}
 static boolean simple(String s){return !s.isEmpty()&&!s.equals(".")&&!s.equals("..")&&s.length()<128&&s.indexOf('/')<0&&s.indexOf('\\')<0&&s.indexOf(':')<0&&s.indexOf('\t')<0&&s.indexOf('\r')<0&&s.indexOf('\n')<0;}

 /** Mission and menu run in different Android processes. Read the published file on each visit. */
 static final class Checkpoint {
  final String mission,difficulty;
  Checkpoint(String mission,String difficulty){this.mission=mission;this.difficulty=difficulty;}

  private static File path(File root,String group){
   String key=CampaignSelection.groupKey(group);
   if(!key.startsWith("missions/")||!CampaignSelection.simple(key.substring(9)))
    throw new IllegalArgumentException("Invalid campaign group");
   return new File(root,"campaign-"+key.substring(9)+".checkpoint");
  }

  static Checkpoint read(File root,String group){
   try(DataInputStream in=new DataInputStream(new FileInputStream(path(root,group)))){
    if(in.readInt()!=1)return null;
    Checkpoint value=new Checkpoint(in.readUTF(),in.readUTF());
    return in.read()==-1?value:null;
   }catch(IOException ignored){return null;}
  }

  void write(File root,String group)throws IOException{
   File target=path(root,group),temp=File.createTempFile("campaign-", ".tmp",root);
   try{
    try(FileOutputStream file=new FileOutputStream(temp);DataOutputStream out=new DataOutputStream(file)){
     out.writeInt(1);out.writeUTF(mission);out.writeUTF(difficulty);out.flush();file.getFD().sync();
    }
    Files.move(temp.toPath(),target.toPath(),StandardCopyOption.ATOMIC_MOVE,StandardCopyOption.REPLACE_EXISTING);
   }finally{Files.deleteIfExists(temp.toPath());}
  }
 }
}
