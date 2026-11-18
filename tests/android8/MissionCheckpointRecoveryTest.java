// Run with the Android 8 android-all framework JAR on the host classpath.
package org.reigncarnated.menu;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;

public final class MissionCheckpointRecoveryTest {
 public static void main(String[] args) throws Exception {
  Path root=Files.createTempDirectory("mission-checkpoint");
  Path base=root.resolve("mission-checkpoint.json");
  Path backup=root.resolve("mission-checkpoint.json.bak");
  MissionCheckpoint checkpoint=new MissionCheckpoint(root.toFile());
  try {
   if(checkpoint.read()!=null)throw new AssertionError("Expected a fresh installation");
   byte[] previous="{\"slot\":\"checkpointA\",\"mission\":\"S01\"}".getBytes(StandardCharsets.UTF_8);
   Files.write(backup,previous);
   if(!"S01".equals(checkpoint.read().getString("mission")))throw new AssertionError("Lost published mission");
   if(!"checkpointB".equals(checkpoint.nextSlot()))throw new AssertionError("Would overwrite recovered slot");
   Files.move(base,backup);
   Files.write(base,new byte[]{123});
   if(!"checkpointA".equals(checkpoint.read().getString("slot")))throw new AssertionError("Read a partial save");
   Files.write(base,new byte[]{123});
   try {checkpoint.nextSlot();throw new AssertionError("Reused slot after corrupt metadata");}
   catch(org.json.JSONException expected) {}
   System.out.println("PASS fresh, backup-only, next slot, partial write and corrupt metadata");
  } finally {
   Files.deleteIfExists(base);Files.deleteIfExists(backup);Files.deleteIfExists(root);
  }
 }
}
