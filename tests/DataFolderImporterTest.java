package org.reigncarnated.menu;
import java.io.*;
import java.nio.file.*;
import java.util.*;

public final class DataFolderImporterTest {
 static int checks;
 static void check(boolean ok,String message){checks++;if(!ok)throw new AssertionError(message);}
 static byte[] pack(String... names)throws IOException{
  ByteArrayOutputStream tableBytes=new ByteArrayOutputStream();DataOutputStream table=new DataOutputStream(tableBytes);
  for(String name:names){byte[] n=name.getBytes("ISO-8859-1");table.writeInt(0);table.writeByte(n.length);table.write(n);table.writeInt(Integer.reverseBytes(56));table.writeInt(0);}
  ByteArrayOutputStream bytes=new ByteArrayOutputStream();DataOutputStream out=new DataOutputStream(bytes);out.writeBytes("NORK");out.writeInt(Integer.reverseBytes(1));out.writeInt(0);out.writeInt(Integer.reverseBytes(56+tableBytes.size()));out.writeInt(Integer.reverseBytes(names.length));out.writeInt(Integer.reverseBytes(56));out.write(new byte[32]);out.write(tableBytes.toByteArray());return bytes.toByteArray();
 }
 static final class Fake implements DataFolderImporter.Source {
  final Map<String,List<DataFolderImporter.Node>> dirs=new HashMap<>();final Map<String,byte[]> files=new HashMap<>();boolean unknown,truncated;int reads;
  DataFolderImporter.Node root=new DataFolderImporter.Node("root","DR2",true,-1);
  Fake(boolean music)throws IOException{
   dir("root","PACKS");dir("root","Missions");dir("root/Missions","System");
   file("root/PACKS","BASE.ZWP",pack("runcode_main_login.cfg","english.cfg"));file("root/Missions/System","Shell.zwp",pack("interface_shell.cfg"));
   if(music){dir("root","Music");file("root/Music","Track10.ogg",new byte[]{10});file("root/Music","Track02.ogg",new byte[]{2});file("root/Music","notes.txt",new byte[]{3});}
  }
  void dir(String parent,String name){String id=parent+"/"+name;dirs.computeIfAbsent(parent,k->new ArrayList<>()).add(new DataFolderImporter.Node(id,name,true,-1));dirs.put(id,new ArrayList<>());}
  void file(String parent,String name,byte[] bytes){String id=parent+"/"+name;dirs.get(parent).add(new DataFolderImporter.Node(id,name,false,bytes.length));files.put(id,bytes);}
  public List<DataFolderImporter.Node> children(DataFolderImporter.Node n){List<DataFolderImporter.Node> result=dirs.get(n.id);if(!unknown)return result;List<DataFolderImporter.Node> copy=new ArrayList<>();for(DataFolderImporter.Node c:result)copy.add(new DataFolderImporter.Node(c.id,c.name,c.directory,-1));return copy;}
  public InputStream open(DataFolderImporter.Node n){reads++;byte[] b=files.get(n.id);return new ByteArrayInputStream(truncated?Arrays.copyOf(b,b.length-1):b);}
 }
 static boolean fails(Fake source,File parent,java.util.function.BooleanSupplier cancel)throws Exception{
  int before=parent.list()==null?0:parent.list().length;try{new DataFolderImporter(source,cancel,s->{}).run(source.root,parent);return false;}catch(IOException expected){check((parent.list()==null?0:parent.list().length)==before,"failed import left a snapshot");return true;}
 }
 public static void main(String[] args)throws Exception{
  File parent=Files.createTempDirectory("reign-import-test-").toFile();
  try{
   Fake source=new Fake(true);DataFolderImporter.Result first=new DataFolderImporter(source,()->false,s->{}).run(source.root,parent);
   check(first.tracks==2,"discover supported music only");check(first.scenePacks==0,"compact menu-only folder remains supported");check(first.directory.getName().startsWith("data-"),"finalized snapshot");
   List<String> tracks=DataFolderImporter.playlist(first.directory);check(tracks.size()==2,"playlist persisted");check(Files.readAllBytes(Paths.get(tracks.get(0)))[0]==2,"track order");
   byte[] original=Files.readAllBytes(new File(first.directory,"base.zwp").toPath());
   Fake noMusic=new Fake(false);DataFolderImporter.Result silent=new DataFolderImporter(noMusic,()->false,s->{}).run(noMusic.root,parent);check(silent.tracks==0,"optional music");
   Fake unknown=new Fake(true);unknown.unknown=true;check(new DataFolderImporter(unknown,()->false,s->{}).run(unknown.root,parent).tracks==2,"unknown provider sizes");
   Fake full=new Fake(true);full.dir("root","Worlds");full.file("root/Worlds","Sprawl.zwp",pack("types_world.cfg"));full.dir("root","Sides");full.dir("root/Sides","Client");full.file("root/Sides/Client","JDA.ZWP",pack("interface_game_side.cfg"));full.file("root/Sides/Client","sprawler.zwp",pack("interface_game_side.cfg"));
   DataFolderImporter.Result scene=new DataFolderImporter(full,()->false,s->{}).run(full.root,parent);check(scene.scenePacks==3,"native shell packs discovered case insensitively");
   check(Arrays.equals(Files.readAllBytes(new File(scene.directory,"worlds/sprawl.zwp").toPath()),full.files.get("root/Worlds/Sprawl.zwp")),"world data preserved exactly");
   check(new File(scene.directory,"sides/client/jda.zwp").isFile()&&new File(scene.directory,"sides/client/sprawler.zwp").isFile(),"scene resource paths preserved");
   full.files.get("root/Worlds/Sprawl.zwp")[0]=0;check(fails(full,parent,()->false),"corrupt optional scene pack fails transaction");
   Fake missing=new Fake(true);missing.dirs.get("root/Missions").clear();check(fails(missing,parent,()->false),"missing required directory");check(missing.reads==0,"preflight before copying");
   Fake duplicate=new Fake(true);duplicate.dirs.get("root/PACKS").add(duplicate.dirs.get("root/PACKS").get(0));check(fails(duplicate,parent,()->false),"duplicate names rejected");
   Fake wrong=new Fake(true);wrong.files.put("root/Missions/System/Shell.zwp",pack("other.cfg"));wrong.unknown=true;check(fails(wrong,parent,()->false),"wrong shell archive rejected");
   Fake broken=new Fake(true);broken.files.get("root/PACKS/BASE.ZWP")[0]=0;check(fails(broken,parent,()->false),"corrupt archive rejected");
   Fake shortRead=new Fake(true);shortRead.truncated=true;check(fails(shortRead,parent,()->false),"short source read rejected");
   Fake cancel=new Fake(true);check(fails(cancel,parent,()->cancel.reads>0),"cancel during copy");
   Fake large=new Fake(true);DataFolderImporter.Node n=large.dirs.get("root/PACKS").get(0);large.dirs.get("root/PACKS").set(0,new DataFolderImporter.Node(n.id,n.name,false,DataFolderImporter.MAX_FILE+1));check(fails(large,parent,()->false),"oversized source rejected");
   check(Arrays.equals(original,Files.readAllBytes(new File(first.directory,"base.zwp").toPath())),"previous snapshot preserved");
   if(args.length>0){
    File installation=new File(args[0]);DataFolderImporter.Source local=new DataFolderImporter.Source(){
     public List<DataFolderImporter.Node> children(DataFolderImporter.Node dir)throws IOException{File[] children=new File(dir.id).listFiles();if(children==null)throw new IOException("Cannot read test directory");List<DataFolderImporter.Node> result=new ArrayList<>();for(File c:children)result.add(new DataFolderImporter.Node(c.getAbsolutePath(),c.getName(),c.isDirectory(),c.length()));return result;}
     public InputStream open(DataFolderImporter.Node file)throws IOException{return new FileInputStream(file.id);}
    };
    DataFolderImporter.Result real=new DataFolderImporter(local,()->false,System.out::println).run(new DataFolderImporter.Node(installation.getAbsolutePath(),installation.getName(),true,-1),parent);
    check(real.tracks==9,"real installation tracks");check(new File(real.directory,"base.zwp").length()==new File(installation,"packs/base.zwp").length(),"real base copied");
    check(real.scenePacks==3,"real world and side packs detected");
    for(String path:new String[]{"worlds/sprawl.zwp","sides/client/jda.zwp","sides/client/sprawler.zwp"})check(Arrays.equals(Files.readAllBytes(new File(real.directory,path).toPath()),Files.readAllBytes(new File(installation,path).toPath())),"real scene bytes preserved: "+path);
    System.out.println("Real installation: base + shell + "+real.tracks+" tracks validated and imported");
   }
   System.out.println("Data folder importer: PASS ("+checks+" checks)");
  }finally{DataFolderImporter.remove(parent);}
 }
}
