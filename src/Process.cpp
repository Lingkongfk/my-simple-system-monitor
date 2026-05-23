#include "Process.h"


std::string Process::Pid()const{
    return pid_;
}
std::string Process::Command()const{
    return procInfo_[0];
}
std::string Process::Ppid()const{
    return procInfo_[2];
}
std::string Process::Status()const{
    return procInfo_[1];
}                

bool Process::setInfo(){
    procInfo_ = LinuxParser::getProcessInfo(pid_);
    if(procInfo_.empty()){
        return false;
    }
    return true;
}

double Process::CpuUtilization(){
    return LinuxParser::CpuUtilizationOfProcess(pid_);
}
void Process::setCpu(double num){
    cpuUtilization_ = num;
}

double Process::getCpu()const{
    return cpuUtilization_;
}

