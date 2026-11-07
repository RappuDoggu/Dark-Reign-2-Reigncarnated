package org.reigncarnated.menu;
import java.io.*;
import java.util.*;
import java.util.zip.*;
/** Explicit collection payload only; never recursively archives app/game storage. */
final class DiagnosticZip {
 static void write(File destination,Map<String,byte[]> entries)throws IOException{
  File temporary=new File(destination.getPath()+".part");boolean complete=false;long total=0;
  try{try(ZipOutputStream zip=new ZipOutputStream(new FileOutputStream(temporary))){for(Map.Entry<String,byte[]> entry:entries.entrySet()){
   String name=entry.getKey();if(!name.matches("[A-Za-z0-9_./-]+")||name.startsWith("/")||Arrays.asList(name.split("/",-1)).contains(".."))throw new IOException("Unsafe diagnostic entry");
   byte[] bytes=entry.getValue();if((total+=bytes.length)>16L*1024*1024)throw new IOException("Diagnostic ZIP exceeds limit");zip.putNextEntry(new ZipEntry(name));zip.write(bytes);zip.closeEntry();
  }}java.nio.file.Files.move(temporary.toPath(),destination.toPath(),java.nio.file.StandardCopyOption.ATOMIC_MOVE,java.nio.file.StandardCopyOption.REPLACE_EXISTING);complete=true;
  }finally{if(!complete)temporary.delete();}
 }
}
