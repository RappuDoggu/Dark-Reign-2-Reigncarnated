package org.reigncarnated.menu;
import java.io.File;
import java.util.Collections;
public final class MissionDataImportTest {
 public static void main(String[] args)throws Exception {
  try(ZipGameSource source=new ZipGameSource(Collections.singletonList(new File(args[0])))){
   DataFolderImporter.Result result=new DataFolderImporter(source,()->false,message->{}).run(source.root(),new File(args[1]));
   for(String name:new String[]{"base.zwp","shell.zwp","library/engine/startup.cfg","missions/multiplayer/2crazed.zwp","worlds/sprawl.zwp","sides/client/sprawler.zwp"})if(!new File(result.directory,name).isFile())throw new AssertionError("Missing imported mission dependency: "+name);
   System.out.println(result.directory.getAbsolutePath());
  }
 }
}
