package org.reigncarnated.menu;

import java.io.*;
import java.util.*;
import java.util.zip.*;

/** Read-only ZIP index. Only known game packs/music are exposed to the importer;
 * ZIP paths are never used as filesystem destinations. Supports wrapped/full and compact ZIPs. */
final class ZipGameSource implements DataFolderImporter.Source, AutoCloseable {
 private final List<ZipFile> archives=new ArrayList<>();
 private final Map<String,DataFolderImporter.Node> nodes=new LinkedHashMap<>();
 private final Map<String,ZipFile> owners=new HashMap<>();
 private final Map<String,ZipEntry> entries=new HashMap<>();
 ZipGameSource(List<File> files)throws IOException {
  try {
   directory("");directory("packs");directory("missions/system");directory("music");
   int count=0;
   for(File file:files){
    ZipFile zip=new ZipFile(file);archives.add(zip);
    Enumeration<? extends ZipEntry> all=zip.entries();
    while(all.hasMoreElements()){
     ZipEntry e=all.nextElement();if(++count>100000)throw new IOException("ZIP contains too many entries");
     String path=e.getName().replace('\\','/');
     if(path.startsWith("/")||path.indexOf(0)>=0||path.contains(":"))throw new IOException("Invalid ZIP path");
     for(String part:path.split("/"))if(part.equals(".."))throw new IOException("Unsafe ZIP path");
     if(e.isDirectory())continue;
     String lower=path.toLowerCase(Locale.ROOT),name=lower.substring(lower.lastIndexOf('/')+1),dest=null;
     if(name.equals("base.zwp"))dest="packs/base.zwp";
     else if(name.equals("shell.zwp"))dest="missions/system/shell.zwp";
     else if(name.equals("sprawl.zwp"))dest="worlds/sprawl.zwp";
     else if(name.equals("jda.zwp")||name.equals("sprawler.zwp"))dest="sides/client/"+name;
     else if((lower.contains("music/")||name.matches("track[0-9]+\\.(ogg|mp3|wav)"))&&(name.endsWith(".ogg")||name.endsWith(".mp3")||name.endsWith(".wav")))dest="music/"+name;
     if(dest==null){
      // Preserve the runtime tree for worlds, missions, side data, movies and mods.
      String[] parts=lower.split("/");
      for(int j=0;j<parts.length-1;j++)if(Arrays.asList("packs","missions","worlds","sides","mods","movies","data","library").contains(parts[j])){dest=String.join("/",Arrays.copyOfRange(parts,j,parts.length));break;}
      if(dest==null&&name.endsWith(".cfg"))dest=name;
      if(dest!=null&&!name.matches(".*\\.(zwp|cfg|bik|ogg|mp3|wav|tga|bmp|pic|god|blk|fff|xsi|txt)"))dest=null;
     }
     if(dest==null)continue;
     if(e.getSize()<0||e.getSize()>DataFolderImporter.MAX_FILE)throw new IOException("Invalid or oversized ZIP entry: "+name);
     if(nodes.containsKey(dest))throw new IOException("Duplicate game file across ZIPs: "+name);
     if(dest.lastIndexOf('/')>=0)directory(dest.substring(0,dest.lastIndexOf('/')));
     nodes.put(dest,new DataFolderImporter.Node(dest,name,false,e.getSize()));owners.put(dest,zip);entries.put(dest,e);
    }
   }
   if(!nodes.containsKey("worlds/sprawl.zwp"))throw new IOException("Missing sprawl.zwp. The live menu needs the Sprawl world pack as well as base.zwp and shell.zwp. Select ZIPs containing all three.");
  }catch(IOException|RuntimeException e){close();throw e;}
 }
 private void directory(String path){if(nodes.containsKey(path))return;int slash=path.lastIndexOf('/');if(slash>=0)directory(path.substring(0,slash));nodes.put(path,new DataFolderImporter.Node(path,slash<0?path:path.substring(slash+1),true,0));}
 DataFolderImporter.Node root(){return nodes.get("");}
 public List<DataFolderImporter.Node> children(DataFolderImporter.Node parent){List<DataFolderImporter.Node> out=new ArrayList<>();for(DataFolderImporter.Node n:nodes.values()){if(n.id.isEmpty())continue;int at=n.id.lastIndexOf('/');String p=at<0?"":n.id.substring(0,at);if(p.equals(parent.id))out.add(n);}return out;}
 public InputStream open(DataFolderImporter.Node node)throws IOException {
  ZipEntry entry=entries.get(node.id);if(entry==null)throw new IOException("Missing ZIP entry");
  return new FilterInputStream(owners.get(node.id).getInputStream(entry)){
   final CRC32 crc=new CRC32();boolean verified;
   private void verify()throws IOException{if(!verified){verified=true;if(crc.getValue()!=entry.getCrc())throw new IOException("ZIP checksum failed: "+node.name);}}
   public int read()throws IOException{int v=in.read();if(v<0)verify();else crc.update(v);return v;}
   public int read(byte[] b,int off,int len)throws IOException{int n=in.read(b,off,len);if(n<0)verify();else crc.update(b,off,n);return n;}
  };
 }
 public void close(){for(ZipFile zip:archives)try{zip.close();}catch(IOException ignored){}archives.clear();}
}

