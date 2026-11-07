package org.reigncarnated.menu;
public final class StartingResourcesTest {
 static int checks;static void check(boolean v){++checks;if(!v)throw new AssertionError("Resource input check "+checks);}
 public static void main(String[] args){check(StartingResources.parse("")==-1);check(StartingResources.parse("  ")==-1);check(StartingResources.parse("0")==0);check(StartingResources.parse(" 44444 ")==44444);check(StartingResources.parse("1000000")==1000000);check(StartingResources.parse("0000010")==10);for(String s:new String[]{"-1","+1","1.5","1e3","1000001","2147483647","12a","1 2"}){boolean bad=false;try{StartingResources.parse(s);}catch(IllegalArgumentException e){bad=true;}check(bad);}check(StartingResources.label(-1).equals("Map default"));check(StartingResources.label(0).equals("0"));System.out.println(checks+" map-default/zero/bounds/integer resource input checks PASS");}
}
