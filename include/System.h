#pragma once
#include "Process.h"
class System
{
public:
    System() = default;
    ~System() = default; 

    double getCPU();//获取CPU的总使用率
    void Update();  //重新采集数据
    std::vector<std::string>& Utilization();//提供cpu使用率，进程CPU占用率
    std::vector<Process>& Processes();//提供进程的信息，包括进程命令，状态，pid，ppid
private:
    //进程数组
    double CPUused;
    std::vector<Process> processes_;//进程数组，可以获得每个进程的基本信息
    std::vector<std::string> utilization_;//获得基本CPU的信息
};

