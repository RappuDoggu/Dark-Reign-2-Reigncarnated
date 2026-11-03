package org.reigncarnated.menu;
import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.zip.*;
public final class GameDataZipTest {
 static int checks;static void check(boolean value){checks++;if(!value)throw new AssertionError("bundle check "+checks);}
 static void put(File root,String path,byte[] bytes)throws Exception{File file=new File(root,path);file.getParentFile().mkdirs();Files.write(file.toPath(),bytes);}
 static void pack(File root,String path,String... names)throws Exception{put(root,path,DataFolderImporterTest.pack(names));}
 static boolean fails(File data,File music,File out,String text)throws Exception{try{GameDataZip.write(data,music,out,"{}",()->false,s->{});return false;}catch(IOException e){return e.getMessage().contains(text);}}
 public static void main(String[] args)throws Exception{
  File root=Files.createTempDirectory("reign-game-bundle-").toFile();try{
   File data=new File(root,"installation"),music=new File(root,"selected-music"),output=new File(root,"bundle.zip");
   pack(data,"base.zwp","runcode_main_login.cfg","english.cfg");pack(data,"shell.zwp","interface_shell.cfg");pack(data,"worlds/sprawl.zwp","types_world.cfg");
   for(String side:new String[]{"jda","sprawler"})pack(data,"sides/client/"+side+".zwp","interface_game_side.cfg");
   put(data,"library/engine/startup.cfg",new byte[]{1});put(data,"missions/multiplayer/group.cfg",new byte[]{1});pack(data,"missions/multiplayer/2crazed.zwp","game.cfg");pack(data,"missions/campaign/first.zwp","game.cfg");put(data,"mods/personality/test.cfg",new byte[]{1});
   put(data,"music/unused.ogg",new byte[]{9});put(music,"music/000.ogg",new byte[]{1,2,3});put(music,"music/001.mp3",new byte[]{4,5,6});
   put(data,"users/profile/data.cfg",new byte[]{10});put(data,"signing.keystore",new byte[]{11});
   GameDataZip.Result result=GameDataZip.write(data,music,output,"{\"source_revision\":\"fixture\"}",()->false,s->{});check(result.tracks==2&&result.file.equals(output)&&result.bytes>0&&result.sha256.length()==64);
   try(ZipFile zip=new ZipFile(output)){check(zip.getEntry("packs/base.zwp")!=null&&zip.getEntry("missions/system/shell.zwp")!=null);check(zip.getEntry("worlds/sprawl.zwp")!=null&&zip.getEntry("music/000.ogg")!=null);check(zip.getEntry("music/unused.ogg")==null);check(zip.getEntry("users/profile/data.cfg")==null&&zip.getEntry("signing.keystore")==null);check(zip.getEntry("data-file-manifest.txt")!=null&&zip.getEntry("apk-source.json")!=null);}
   try(ZipGameSource source=new ZipGameSource(Collections.singletonList(output))){DataFolderImporter.Result imported=new DataFolderImporter(source,()->false,s->{}).run(source.root(),new File(root,"imports"));check(imported.tracks==2&&imported.scenePacks==3);check(Arrays.equals(Files.readAllBytes(new File(imported.directory,"worlds/sprawl.zwp").toPath()),Files.readAllBytes(new File(data,"worlds/sprawl.zwp").toPath())));check(Arrays.equals(Files.readAllBytes(new File(imported.directory,"music/000.ogg").toPath()),new byte[]{1,2,3}));check(new File(imported.directory,"mods/personality/test.cfg").isFile()&&new File(imported.directory,"library/engine/startup.cfg").isFile()&&new File(imported.directory,"missions/campaign/first.zwp").isFile());}
   byte[] previous=Files.readAllBytes(output.toPath());File world=new File(data,"worlds/sprawl.zwp"),away=new File(root,"world.saved");check(world.renameTo(away));check(fails(data,music,output,"Missing worlds/sprawl.zwp"));check(Arrays.equals(previous,Files.readAllBytes(output.toPath())));check(away.renameTo(world));
   check(GameDataZip.musicRoot(data,new File(root,"missing")).equals(data));
   GameDataZip.Result fallback=GameDataZip.write(data,new File(root,"missing"),new File(root,"fallback.zip"),"{}",()->false,s->{});check(fallback.tracks==1);
   new File(data,"music/unused.ogg").delete();check(fails(data,null,output,"No music"));check(Arrays.equals(previous,Files.readAllBytes(output.toPath())));
   put(data,"music/restored.ogg",new byte[]{9});
   AtomicBoolean stop=new AtomicBoolean();try{GameDataZip.write(data,music,output,"{}",stop::get,s->stop.set(true));throw new AssertionError("cancel accepted");}catch(IOException expected){check(expected.getMessage().contains("cancelled"));}
   check(Arrays.equals(previous,Files.readAllBytes(output.toPath())));check(Arrays.stream(root.listFiles()).noneMatch(f->f.getName().endsWith(".part")));
   check(fails(data,music,new File(data,"bad.zip"),"outside"));
   File link=new File(data,"library/secret.txt");Files.createSymbolicLink(link.toPath(),new File(data,"signing.keystore").toPath());check(fails(data,music,output,"links"));Files.delete(link.toPath());
   File large=new File(data,"mods/large.txt");try(RandomAccessFile f=new RandomAccessFile(large,"rw")){f.setLength(DataFolderImporter.MAX_FILE+1);}check(fails(data,music,output,"oversized"));large.delete();
   byte[] valid=Files.readAllBytes(world.toPath());valid[0]=0;Files.write(world.toPath(),valid);check(fails(data,music,output,"Invalid Dark Reign 2 archive"));check(Arrays.equals(previous,Files.readAllBytes(output.toPath())));
   System.out.println("Complete game bundle: canonical packs + separate music, real importer round-trip, private-file exclusion, missing Sprawl/music, rollback, cancellation, links and size guards PASS ("+checks+" checks)");
  }finally{DataFolderImporter.remove(root);}
 }
}
