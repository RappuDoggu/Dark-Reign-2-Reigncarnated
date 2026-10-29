#include "mission_prelude.h"
#include "type_ident_registry.h"
#include "stdload.h"
#include "system_sync.h"
#include "native_stack.h"
#include "clock.h"
#include "text_native.h"
#include "debug_native.h"
#include <thread>
#include <iostream>
#include <vector>
#include <atomic>
#include <cfenv>
#include <climits>
static void check(bool value) { if(!value) throw std::runtime_error("Native runtime invariant"); }
int main() {
 try {
  using namespace NativeEngine;
  check(Utils::Div(U32(1),U32(0),U32(2))==0x80000000u);
  bool zero=false,overflow=false;
  try { Utils::Div(U32(0),U32(1),U32(0)); } catch(const std::domain_error &) {zero=true;}
  try { Utils::Div(U32(1),U32(0),U32(1)); } catch(const std::overflow_error &) {overflow=true;}
  check(zero && overflow);
  const int initialRound=std::fegetround();std::feraiseexcept(FE_INVALID);
  Utils::FP::Reset();check(std::fegetround()==FE_TOWARDZERO && !std::fetestexcept(FE_ALL_EXCEPT));std::fesetround(initialRound);
  CH integer[34];
  check(Utils::ItoA(INT_MIN,integer,10)==integer && Utils::Strcmp(integer,REIGN_TEXT16("-2147483648"))==0);
  Utils::ItoA(-1,integer,16);check(Utils::Strcmp(integer,REIGN_TEXT16("ffffffff"))==0);
  Utils::ItoA(0,integer,2);check(integer[0]=='0' && integer[1]==0);
  Utils::ItoA(-1,integer,2);check(Utils::Strlen(integer)==32);

  check(Utils::Strnicmp("AbC-tail","aBc-other",3)==0);
  check(Utils::Strnicmp("a","AB",3)<0);
  check(Utils::Strnicmp("z","Y",1)>0);
  check(Utils::FtoL(-1.9f)==-1 && Utils::FtoL(1.9f)==1);
  check(Utils::FtoLUpProc(-1.9f)==-1 && Utils::FtoLDownProc(-1.1f)==-2);
  check(Utils::FtoLNearestProc(2.5f)==2 && Utils::FtoLNearestProc(3.5f)==4);
  const auto savedMode=Utils::FP::SetRoundDownModeProc();
  check(Utils::FastFtoLProc(1.9f)==1);
  Utils::FP::SetRoundUpModeProc();check(Utils::FastFtoLProc(1.1f)==2);
  check(Utils::FtoLNearestProc(2.5f)==2 && Utils::FastFtoLProc(1.1f)==2);
  Utils::FP::RestoreModeProc(savedMode);
  check(Utils::FtoL(std::numeric_limits<float>::infinity())==std::numeric_limits<S32>::min());
  const CH *literalA=REIGN_TEXT16("AB");const CH *literalB=REIGN_TEXT16("CD");
  check(literalA[0]=='A' && literalB[0]=='C' && literalA!=literalB);
  const CH *supplementary=REIGN_TEXT16("\U0001F600");check(supplementary[0]==0xd83d&&supplementary[1]==0xde00&&supplementary[2]==0);
  CH text[128],encoded[16];char decoded[16];
  check(Utils::Ansi2Unicode(encoded,16,"A\x80\xf1")==4);
  check(encoded[1]==0x20ac && encoded[2]==0xf1);
  check(Utils::Unicode2Ansi(decoded,16,encoded)==4 && std::memcmp(decoded,"A\x80\xf1",4)==0);
  const CH fmt[]={'%','s',' ', '%','0','4','d',' ', '%','.','2','f',' ', '%','l','l','u',0};
  const CH word[]={0x03a9,0x4e16,0x754c,0};
  check(Utils::Sprintf(text,128,fmt,word,-12,1.25,0x100000001ull)>0);
  check(text[0]==0x03a9 && text[1]==0x4e16);
  check(std::strstr(Utils::Unicode2Ansi(text),"-012 1.25 4294967297")!=nullptr);
  const CH clipped[]={'%','s',0};CH small[3];
  check(Utils::Sprintf(small,3,clipped,word)==-1 && small[0]==word[0] && small[1]==word[1] && small[2]==0);
  CH copy[5]={1,1,1,1,1};Utils::Strncpy(copy,word,5);check(copy[3]==0&&copy[4]==0);
  U8 bytes[2];Utils::CharDecode(bytes,"1aEF",4,2);check(bytes[0]==0xa1 && bytes[1]==0xfe);
  char hex[5];Utils::CharEncode(hex,bytes,2,5);check(std::strcmp(hex,"1AEF")==0);
  bool oddHexRejected=false;try{Utils::CharDecode(bytes,"1",1,2);}catch(const std::length_error &){oddHexRejected=true;}check(oddHexRejected);
  check(std::strcmp(Utils::StrFmtHex(nullptr,4,0x2a,0),"002A")==0);
  Clock::Time::Init();
  const auto before=Clock::Time::Us();
  check(Clock::Time::Us()>=before);
  check(Clock::Date::DayOfWeek(2000,2,29)==3);
  check(Clock::Date::DayOfWeek(2024,2,1)==5);
  check(Clock::Date::DayOfWeek(2024,3,1)==6);
  check(Clock::Date::DaysInMonth(2000,2)==29);
  check(Clock::Date::DaysInMonth(2100,2)==28);
  check(std::strlen(Clock::Date::GetVerbose())>10);
  char timeText[9],dateText[9];Clock::Time::GetStr(timeText);Clock::Date::GetStr(dateText);
  check(std::strlen(timeText)==8 && std::strlen(dateText)==8);
  Clock::Watch watch;watch.Sample(0xffffffffu);watch.Sample(0xffffffffu);
  check(watch.GetSmooth()==0xffffffffu && watch.GetAvg()==0xffffffffu);
  check(watch.GetSum()==0x1fffffffeull);
  Clock::CycleWatch timer;timer.Start();timer.Start();timer.Stop();timer.Stop();
  check(timer.GetSamples()==1);timer.Reset();check(timer.GetSamples()==0);
  struct Target {};
  FScope config(nullptr,"Type");config.AddArgString("original.loader.type");config.InitIterators();
  Reaper<Target> loaded;
  StdLoad::TypeReaperObjType(&config,loaded);
  check(loaded.HasResolveId());
  check(std::strcmp(TakeTypeIdent(loaded.GetResolveId()).str,"original.loader.type")==0);
  auto first=StoreTypeIdent("sprawler.constructor");
  auto second=StoreTypeIdent("jda.hq");
  check(first!=second && PendingTypeIdents()==2);
  check(std::strcmp(TakeTypeIdent(first).str,"sprawler.constructor")==0);
  bool rejected=false;try { TakeTypeIdent(first); } catch(const std::logic_error &) { rejected=true; }
  check(rejected);
  ClearTypeIdents();
  rejected=false;try { TakeTypeIdent(second); } catch(const std::logic_error &) { rejected=true; }
  check(rejected && PendingTypeIdents()==0);
  check(StoreTypeIdent("next.mission")>second);ClearTypeIdents();
  std::vector<std::thread> workers;
  for(int i=0;i<4;++i)workers.emplace_back([] {
   for(int n=0;n<500;++n) {
    auto token=StoreTypeIdent("concurrent.type");
    check(std::strcmp(TakeTypeIdent(token).str,"concurrent.type")==0);
   }
  });
  for(auto &worker:workers)worker.join();
  check(PendingTypeIdents()==0);
  System::Semaphore ready(0,1), consumed(0,1);
  System::Mutex mutex;
  int value=0;
  check(!ready.Wait(0));
  std::thread consumer([&] { check(ready.Wait(2000));check(mutex.Wait());check(value==42);mutex.Signal();consumed.Signal(); });
  check(mutex.Wait());value=42;mutex.Signal();ready.Signal();
  check(consumed.Wait(2000));consumer.join();check(!ready.Test());
  ready.Signal();rejected=false;
  try { ready.Signal(); } catch(const std::overflow_error &) { rejected=true; }
  check(rejected && ready.Wait(0));
  check(mutex.Wait() && mutex.Wait(0));mutex.Signal();mutex.Signal();
  std::uintptr_t frames[32];check(CaptureStack(frames,32)>0);
  if(std::strcmp(Utils::ChopPath("data\\models/unit.god"),"unit.god") || std::strcmp(Utils::ChopPath("unit.god"),"unit.god") || *Utils::ChopPath("data/")) throw std::runtime_error("Path basename failed");
  Debug::Error::Set("mission.cfg",42,"test",Debug::Error::CONFIG);
  std::thread diagnostic([] { Debug::Error::Set("worker.cfg",9,"test",Debug::Error::FATAL); });diagnostic.join();
  bool diagnosticCaught=false;
  try { Debug::Error::Err("Missing type %s", "test.unit"); }
  catch(const std::runtime_error &error) { diagnosticCaught=std::string(error.what()).find("mission.cfg:42 [1] Missing type test.unit")!=std::string::npos; }
  check(diagnosticCaught);
  Debug::Watchdog::Create();Debug::Watchdog::Poll();Debug::Watchdog::Delete();
  Debug::Watchdog::Enable();
  for(int i=0;i<3;++i){Debug::Watchdog::Create();Debug::Watchdog::Create();Debug::Watchdog::Poll();Debug::Watchdog::Delete();}
  std::cout<<"Type identifiers, native queues, timing/calendar, string comparison and unwinding passed\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
