package org.reigncarnated.menu;
final class StartingResources {
 static final int MAP_DEFAULT=-1,MAX=1000000;
 static int parse(String text){String s=text.trim();if(s.isEmpty())return MAP_DEFAULT;if(s.length()>7)throw new IllegalArgumentException("Enter 0–1,000,000 or leave blank for map default");long value=0;for(int i=0;i<s.length();++i){char c=s.charAt(i);if(c<'0'||c>'9')throw new IllegalArgumentException("Use whole resource amounts only");value=value*10+c-'0';}if(value>MAX)throw new IllegalArgumentException("Maximum starting resources is 1,000,000");return (int)value;}
 static String label(int value){return value==MAP_DEFAULT?"Map default":Integer.toString(value);}
}
