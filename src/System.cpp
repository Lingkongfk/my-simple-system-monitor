#include "System.h"
#include <chrono>
#include <cstdio>
#include <mutex>
#include <string>
#include <thread>
#include <cmath>
#include <type_traits>
#include <vector>
#include <utility>

//得到单调递增的精确时间
double getMonotonicTime(){
     struct timespec ts;
     clock_gettime(CLOCK_MONOTONIC, &ts);
     return ts.tv_sec + ts.tv_nsec / 1.0e9;
}


void System::Update(){
    //CPU信息更新
    double t1 = getMonotonicTime();

    std::vector<std::string> cpuinfo = LinuxParser::CpuUtilization();
    double idle1 = stod(cpuinfo[0]);
    double total1 = stod(cpuinfo[1]);
   
    std::vector<std::string> temp_utilization;
    temp_utilization.push_back(cpuinfo[2]);
    temp_utilization.push_back(cpuinfo[3]);
    temp_utilization.push_back(cpuinfo[4]);


    std::vector<Process> temp_processes;
    std::vector<double> procTime;
    //重新采集运行进程的pid，并且创建进程信息结构体
    std::vector<std::string> pids = LinuxParser::Pids();
    for(int i=0;i<pids.size();i++){
        Process p(pids[i]);

        //如果进程信息设置成功再录入
        if(p.setInfo()){
            temp_processes.push_back(std::move(p));
            //开启采样
            procTime.push_back(temp_processes[i].CpuUtilization());
        }
    }

    //采样间隔
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    double t2 = getMonotonicTime();
    std::vector<std::string> cpuinfo2 = LinuxParser::CpuUtilization();
    double idle2 = stod(cpuinfo2[0]);
    double total2 = stod(cpuinfo2[1]);

    double temp_CPUused = ((total2 - idle2) - (total1 - idle1)) / (total2 - total1) * 100;

    for(int i=0;i<procTime.size();i++){
        double procTime2 = temp_processes[i].CpuUtilization();
        temp_processes[i].setCpu((procTime2 - procTime[i]) / (100 * (t2 - t1)) * 100);
    }


    //最后上锁数据更新
    {
        std::unique_lock<std::mutex> lock(mtx_);
        utilization_ = temp_utilization;
        processes_ = temp_processes;
        CPUused = temp_CPUused;
    }
}
std::vector<std::string> System::Utilization(){
    return utilization_;
}
std::vector<Process> System::Processes(){
    return processes_;
}

double System::getCPU(){
    return CPUused;
}
