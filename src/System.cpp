#include "System.h"
#include "LinuxParser.h"
#include <chrono>
#include <cstdio>
#include <mutex>
#include <string>
#include <sys/unistd.h>
#include <thread>
#include <cmath>
#include <type_traits>
#include <vector>
#include <utility>
#include <climits>
#include <algorithm>
//得到单调递增的精确时间
double getMonotonicTime(){
     struct timespec ts;
     clock_gettime(CLOCK_MONOTONIC, &ts);
     return ts.tv_sec + ts.tv_nsec / 1.0e9;
}

void sortByField(std::vector<Process>& procs, SORT_BY now_by, bool desc){
    //比较字符串
    auto cmpString = [](const std::string& l, const std::string& r)->bool{
        if(l.size() != r.size()){
            return l.size() < r.size();
        }
        for(int i=0;i<l.size();i++){
            if(l[i]!=r[i]){
                return l[i] < r[i];
            }
        }
        return true;
    };

    if(desc){
        sort(procs.begin(), procs.end(), [now_by, cmpString](const Process& l, const Process& r)->bool{
            if(now_by == SORT_BY::BY_CPU) return (l.getCpu() == r.getCpu()? cmpString(l.Pid(), r.Pid()):l.getCpu() > r.getCpu());
            else if(now_by == SORT_BY::BY_PID) return cmpString(r.Pid(), l.Pid()); 
            else if(now_by == SORT_BY::BY_MEM) return (l.getMem() == r.getMem()? cmpString(l.Pid(), r.Pid()) : l.getMem() > r.getMem()); 
            else return true;
        });
    }else{
        sort(procs.begin(), procs.end(), [now_by, cmpString](const Process& l, const Process& r)->bool{
            if(now_by == SORT_BY::BY_CPU) return (l.getCpu() == r.getCpu()? cmpString(l.Pid(), r.Pid()):l.getCpu() < r.getCpu());
            else if(now_by == SORT_BY::BY_PID) return cmpString(l.Pid(), r.Pid()); 
            else if(now_by == SORT_BY::BY_MEM) return (l.getMem() == r.getMem()? cmpString(l.Pid(), r.Pid()) : l.getMem() < r.getMem()); 
            else return true;
        });
    }
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
            procTime.push_back(temp_processes.back().CpuUtilization());
        }
    }

    //采样间隔
    //std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    double t2 = getMonotonicTime();
    std::vector<std::string> cpuinfo2 = LinuxParser::CpuUtilization();
    double idle2 = stod(cpuinfo2[0]);
    double total2 = stod(cpuinfo2[1]);

    double temp_CPUused = ((total2 - idle2) - (total1 - idle1)) / (total2 - total1) * 100;

    for(int i=0;i<procTime.size();i++){
        double procTime2 = temp_processes[i].CpuUtilization();
        temp_processes[i].setCpu((procTime2 - procTime[i]) / (100 * (t2 - t1)) * 100);
    }

    std::vector<std::string> meminfo = LinuxParser::MemoryUtilization();
    std::string kernel = LinuxParser::Kernel();
    std::string os_releas = LinuxParser::OperatingSystem();
    double totalMem = INT_MAX;
    if(meminfo.size() > 0){
        totalMem = stod(meminfo[0]);
    }
    for(int i=0;i<temp_processes.size();i++){
        double rss = temp_processes[i].MemUtilization();
        double rss_kb = rss * sysconf(_SC_PAGESIZE) / 1024;
        temp_processes[i].setMem(rss_kb / totalMem * 100);
    }
    //新数据放在缓冲里面


    SORT_BY current_sort = sort_by_.load();
    bool current_desc = desc_.load();

    sortByField(temp_processes, current_sort, current_desc);    

    //最后上锁数据更新
    {
        std::unique_lock<std::mutex> lock(mtx_);
        hasUpdate = true;
        std::swap(temp_utilization, utilization_back_);
        std::swap(temp_processes, processes_back_);
        std::swap(temp_CPUused , CPUused_back_);
        std::swap(meminfo, meminfo_back_);
        std::swap(kernel, kernel_back_);
        std::swap(os_releas, os_release_back_);
        
    }
}
std::vector<std::string>& System::Utilization(){
    if(hasUpdate){
        std::lock_guard<std::mutex> lock(mtx_);
        std::swap(utilization_back_, utilization_);
    }
    return utilization_;
}

//提供给UI
std::vector<Process>& System::Processes(){
    if(hasUpdate){
        std::lock_guard<std::mutex> lock(mtx_);
        std::swap(processes_, processes_back_);
    }
    return processes_;
}

double& System::getCPU(){
    if(hasUpdate){
        std::lock_guard<std::mutex> lock(mtx_);
        std::swap(CPUused_back_, CPUused);
    }
    hasUpdate = false;
    return CPUused;
}

std::vector<std::string>& System::meminfo(){
    if(hasUpdate){
        std::lock_guard<std::mutex> lock(mtx_);
        std::swap(meminfo_back_, meminfo_);
    }
    return meminfo_;
}

std::string& System::Kernel(){
    if(hasUpdate){
        std::lock_guard<std::mutex> lock(mtx_);
        std::swap(kernel_, kernel_back_);
    }
    return kernel_;
}

std::string& System::os_release(){
    if(hasUpdate){
        std::lock_guard<std::mutex> lock(mtx_);
        std::swap(os_release_, os_release_back_);
    }
    return os_release_;
}
