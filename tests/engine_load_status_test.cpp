#include "load_status.h"
#include <thread>
#include <stdexcept>
#include <iostream>
int main(){NativeEngine::LoadStatus status;status.Reset();std::thread writer([&]{for(unsigned i=0;i<200;++i)status.Update("stage "+std::to_string(i));status.Complete();});for(unsigned i=0;i<200;++i)if(status.Report().empty())throw std::runtime_error("Concurrent loading report is empty");writer.join();auto report=status.Report();if(report.rfind("READY\nSimulation running\n",0)!=0||report.find("stage 199")==std::string::npos||report.find("ms: stage 0\n")!=std::string::npos)throw std::runtime_error("Bounded loading history or completion failed");status.Reset();if(status.Report().rfind("LOADING\nNative startup\n",0)!=0)throw std::runtime_error("Loading reset retained old readiness");std::cout<<"Loading report remains available across concurrent updates; bounded history, completion and reset PASS\n";}
