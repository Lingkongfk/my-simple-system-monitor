#pragma once
#include "Process.h"
#include <mutex>
#include <atomic>

enum class SORT_BY {BY_CPU, BY_PID, BY_MEM};

class System
{
public:
    System() = default;
    ~System() = default; 
    void lock(){
        mtx_.lock();
    }
    void unlock(){
        mtx_.unlock();
    }

    double& getCPU();//获取CPU的总使用率
    void Update();  //重新采集数据
    std::vector<std::string>& Utilization();//提供cpu使用率，进程CPU占用率
    std::vector<Process>& Processes();//提供进程的信息，包括进程命令，状态，pid，ppid
    std::vector<std::string>& meminfo();//提供内存信息
    std::string& Kernel();//提供内核信息
    std::string& os_release();//提供发行版信息
                              
    SORT_BY GetSortBy()const{return sort_by_.load();}
    void SetSortBy(SORT_BY by){sort_by_.store(by);}
    bool IsDesc()const {return desc_.load();}
    void ToggleDesc(){desc_.store(!desc_.load());}

private:
    //进程数组
    double CPUused;
    double CPUused_back_;
    std::string os_release_;
    std::string os_release_back_;
    std::string kernel_;
    std::string kernel_back_;
    std::vector<std::string> meminfo_;
    std::vector<std::string> meminfo_back_;
    std::vector<Process> processes_;//进程数组，可以获得每个进程的基本信息
    std::vector<Process> processes_back_;//双缓冲机制
    std::vector<std::string> utilization_;//获得基本CPU的信息
    std::vector<std::string> utilization_back_;
    std::mutex mtx_;//互斥锁，线程安全访问System内部信息
    
    //数据已经更新
    std::atomic_bool hasUpdate;

    //原子变量保证线程安全
    std::atomic<SORT_BY> sort_by_ {SORT_BY::BY_PID};
    std::atomic_bool desc_{false};
};

//排序
void sortByField(std::vector<Process>& procs, SORT_BY now_by, bool desc);

