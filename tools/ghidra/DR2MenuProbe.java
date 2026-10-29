// Original Reigncarnated tooling. Output includes private decompiler text: do not publish it.
// @category Reigncarnated
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import com.google.gson.*;
import java.nio.file.*;

public class DR2MenuProbe extends GhidraScript {
 public void run() throws Exception {
  if(getScriptArgs().length!=1)throw new IllegalArgumentException("Supply a NEW private output directory");
  String hash="5ea4734ab2f2ad381877ce1c0d9b03365ddb3dd8e3ff0719a634296be1c15250";
  if(!hash.equalsIgnoreCase(currentProgram.getExecutableSHA256()) ||
     !"x86:LE:32:default".equals(currentProgram.getLanguageID().toString()))
   throw new IllegalArgumentException("Addresses are only verified for the supplied community executable");
  Path out=Paths.get(getScriptArgs()[0]);Files.createDirectory(out);
  // First six addresses come from the independently checked WinMain registration sequence.
  // Remaining addresses are direct callees; names remain hypotheses until source comparison.
  String[] addresses={"005456d0","00545890","005455a0","00545620","00545840","00545510",
                      "00558d20","00548360","005e7610","005e7160","00550480","005e56c0","005572e0"};
  JsonObject report=new JsonObject();report.addProperty("executable_sha256",hash);
  report.addProperty("language",currentProgram.getLanguageID().toString());
  report.addProperty("engine_executed",false);report.addProperty("menu_rendered",false);
  JsonArray rows=new JsonArray();DecompInterface dc=new DecompInterface();
  try {
   if(!dc.openProgram(currentProgram))throw new IllegalStateException(dc.getLastMessage());
   for(String a:addresses) {
    monitor.checkCancelled();Function f=getFunctionAt(toAddr(a));
    if(f==null){disassemble(toAddr(a));f=createFunction(toAddr(a),null);}
    if(f==null)throw new IllegalStateException("No function at "+a);
    DecompileResults r=dc.decompileFunction(f,120,monitor);
    if(!r.decompileCompleted() || r.getDecompiledFunction()==null)throw new IllegalStateException(r.getErrorMessage());
    String c=r.getDecompiledFunction().getC();
    Files.writeString(out.resolve(a+".c.txt"),c,StandardOpenOption.CREATE_NEW);
    JsonObject row=new JsonObject();row.addProperty("address",a);row.addProperty("decompiled",true);
    row.addProperty("pseudocode_characters",c.length());JsonArray calls=new JsonArray();
    InstructionIterator instructions=currentProgram.getListing().getInstructions(f.getBody(),true);
    while(instructions.hasNext()) {
     Instruction i=instructions.next();
     for(Reference ref:i.getReferencesFrom())if(ref.getReferenceType().isCall()) {
      JsonObject edge=new JsonObject();edge.addProperty("site",i.getAddress().toString());
      edge.addProperty("target",ref.getToAddress().toString());calls.add(edge);
     }
    }
    row.add("direct_call_references",calls);rows.add(row);
    println("MENU_CALLBACK "+a+" decompiled=true characters="+c.length());
   }
  } finally {dc.dispose();}
  report.add("functions",rows);
  // Written only after ALL functions succeed; headless exit status alone is insufficient.
  Files.writeString(out.resolve("menu-metadata.json"),new GsonBuilder().setPrettyPrinting().create().toJson(report)+"\n",StandardOpenOption.CREATE_NEW);
 }
}
