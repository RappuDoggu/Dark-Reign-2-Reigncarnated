package org.reigncarnated.menu;
import java.util.*;
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
 private static boolean bit(String s){return s.equals("0")||s.equals("1");}
 private static boolean simple(String s){return !s.isEmpty()&&!s.equals(".")&&!s.equals("..")&&s.length()<128&&s.indexOf('/')<0&&s.indexOf('\\')<0&&s.indexOf(':')<0&&s.indexOf('\t')<0&&s.indexOf('\r')<0&&s.indexOf('\n')<0;}
}
