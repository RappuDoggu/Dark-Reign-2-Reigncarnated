// Original Reigncarnated analysis tooling; no game implementation.
// @category Reigncarnated
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import com.google.gson.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

public class DR2StartupProbe extends GhidraScript {
 public void run() throws Exception {
  Path out=Paths.get(getScriptArgs()[0]);
  if(!Files.isDirectory(out)) throw new IllegalArgumentException("Use an existing private output directory");
  if(Files.exists(out.resolve("startup-metadata.json"))) throw new IllegalArgumentException("Evidence already exists; use a new directory");
  JsonObject report=new JsonObject();
  report.addProperty("program",currentProgram.getName());
  report.addProperty("language",currentProgram.getLanguageID().toString());
  report.addProperty("executable_sha256",currentProgram.getExecutableSHA256());
  JsonArray refs=new JsonArray();
  Map<Function,Set<String>> candidates=new HashMap<>();
  for(String word:new String[]{"KeyCheck","Intro","Login","Shell","Mission","Outro","Studio"}) {
   byte[] pattern=(word+"\0").getBytes(StandardCharsets.US_ASCII);
   Address cursor=currentProgram.getMinAddress();
   while(cursor!=null) {
    Address found=currentProgram.getMemory().findBytes(cursor,pattern,null,true,monitor);
    if(found==null) break;
    for(Reference ref:getReferencesTo(found)) {
     Function f=getFunctionContaining(ref.getFromAddress());
     if(f==null) continue;
     candidates.computeIfAbsent(f,k->new TreeSet<>()).add(word);
     JsonObject row=new JsonObject();row.addProperty("string",word);row.addProperty("string_address",found.toString());row.addProperty("reference",ref.getFromAddress().toString());row.addProperty("function",f.getEntryPoint().toString());refs.add(row);
    }
    cursor=found.equals(currentProgram.getMaxAddress())?null:found.next();
   }
  }
  report.add("string_references",refs);
  JsonArray selected=new JsonArray();
  DecompInterface dc=new DecompInterface();
  try {
   if(!dc.openProgram(currentProgram))throw new IllegalStateException(dc.getLastMessage());
   for(Map.Entry<Function,Set<String>> e:candidates.entrySet()) {
    if(e.getValue().size()<6)continue;
    Function f=e.getKey();JsonObject row=new JsonObject();row.addProperty("address",f.getEntryPoint().toString());row.addProperty("name",f.getName());row.add("run_code_strings",new Gson().toJsonTree(e.getValue()));
    DecompileResults result=dc.decompileFunction(f,120,monitor);row.addProperty("decompile_completed",result.decompileCompleted());row.addProperty("error",result.getErrorMessage());
    if(result.decompileCompleted()) {
     String code=result.getDecompiledFunction().getC();
     Files.writeString(out.resolve("startup-"+f.getEntryPoint()+".c.txt"),code,StandardOpenOption.CREATE_NEW);
     row.addProperty("pseudocode_characters",code.length());
     println("DR2_STARTUP_DECOMPILE address="+f.getEntryPoint()+" run_codes="+e.getValue()+" characters="+code.length());
    }
    selected.add(row);
   }
  } finally {dc.dispose();}
  report.add("startup_candidates",selected);
  Files.writeString(out.resolve("startup-metadata.json"),new GsonBuilder().setPrettyPrinting().create().toJson(report)+"\n",StandardOpenOption.CREATE_NEW);
  if(selected.size()!=1)throw new IllegalStateException("Expected exactly one candidate; got "+selected.size());
 }
}
