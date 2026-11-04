#include "load_status.h"
#include <thread>
#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
int main(){
 NativeEngine::LoadStatus status;status.Reset();std::thread writer([&]{for(unsigned i=0;i<200;++i)status.Update("stage "+std::to_string(i));status.Complete();});for(unsigned i=0;i<200;++i)if(status.Report().empty())throw std::runtime_error("Concurrent loading report is empty");writer.join();auto report=status.Report();if(report.rfind("READY\nSimulation running\n",0)!=0||report.find("stage 199")==std::string::npos||report.find("ms: stage 0\n")!=std::string::npos)throw std::runtime_error("Bounded loading history or completion failed");status.Reset();if(status.Report().rfind("LOADING\nNative startup\n",0)!=0)throw std::runtime_error("Loading reset retained old readiness");
 auto path=std::filesystem::temp_directory_path()/("reign-journal-test-"+std::to_string(getpid())+".log");
 auto child=fork();if(child<0)throw std::runtime_error("Cannot create crash-journal fixture");if(child==0){NativeEngine::LoadStatus pending;pending.Reset();pending.ConfigureJournal(path);pending.Update("Loading scripted campaign objects");_exit(17);}int result=0;if(waitpid(child,&result,0)!=child||!WIFEXITED(result)||WEXITSTATUS(result)!=17)throw std::runtime_error("Fixture exit failed");
 std::ifstream in(path);std::string bytes((std::istreambuf_iterator<char>(in)),{});if(bytes.find("Loading scripted campaign objects")==std::string::npos)throw std::runtime_error("Last phase did not survive a process exit without cleanup");in.close();
 status.ConfigureJournal(path);for(unsigned i=0;i<1000;++i)status.Update(std::string(500,'x')+std::to_string(i));if(std::filesystem::file_size(path)>100*1024)throw std::runtime_error("Persistent journal exceeded its bound");std::filesystem::remove(path);
 std::cout<<"Concurrent bounded loading history, reset and persistent last stage surviving immediate process exit PASS\n";
}
