#include "System.h"
#include <chrono>
#include <string>
#include <thread>
#include <cmath>


//得到单调递增的精确时间
double getMonotonicTime(){
     struct timespec ts;
     clock_gettime(CLOCK_MONOTONIC, &ts);
     return ts.tv_sec + ts.tv_nsec / 1.0e9;
}


void System::Update(){

    //CPU信息更新
    utilization_.clear();
    processes_.clear();
    double t1 = getMonotonicTime();

    std::vector<std::string> cpuinfo = LinuxParser::CpuUtilization();
    double idle1 = stod(cpuinfo[0]);
    double total1 = stod(cpuinfo[1]);
    utilization_.push_back(cpuinfo[2]);
    utilization_.push_back(cpuinfo[3]);
    utilization_.push_back(cpuinfo[4]);

    //刷新数据，先把进程数组清空
    processes_.clear();
    std::vector<double> procTime;
    //重新采集运行进程的pid，并且创建进程信息结构体
    std::vector<std::string> pids = LinuxParser::Pids();
    for(int i=0;i<pids.size();i++){
        processes_.emplace_back(pids[i]);
        //设置进程信息结构体的信息
        processes_[i].setInfo();

        //开启采样
        procTime.push_back(processes_[i].CpuUtilization());
    }


    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    double t2 = getMonotonicTime();
    std::vector<std::string> cpuinfo2 = LinuxParser::CpuUtilization();
    double idle2 = stod(cpuinfo2[0]);
    double total2 = stod(cpuinfo2[1]);

    CPUused = ((total2 - idle2) - (total1 - idle1)) / (total2 - total1) * 100;

    for(int i=0;i<procTime.size();i++){
        double procTime2 = processes_[i].CpuUtilization();
        processes_[i].setCpu((procTime2 - procTime[i]) / (100 * (t2 - t1)) * 100);
    }

}
std::vector<std::string>& System::Utilization(){
    return utilization_;
}
std::vector<Process>& System::Processes(){
    return processes_;
}

double System::getCPU(){
    return CPUused;
}
