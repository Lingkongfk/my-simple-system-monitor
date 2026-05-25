#include <chrono>
#include <iostream>
#include "Process.h"
#include "System.h"
#include <algorithm>
#include <mutex>
#include <sys/unistd.h>
#include <thread>
#include <atomic>

//声明全局变量
System s;
std::atomic_bool flag;//判断是否退出

//信息采集线程，每隔1s调用Update()，更新数据
void Collector(){
    while(true){
        if(flag){
            break;
        }
        s.Update();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

//UI线程，每隔200ms读取一次数据并且刷新显示
void Display(){
    while(true){
        if(flag){
            break;
        }
        std::vector<std::string> info = s.Utilization();
        std::vector<Process> procs = s.Processes();
        std::sort(procs.begin(), procs.end(), [](const Process& l, const Process& r)->bool{ 
            if(l.getCpu() == r.getCpu()){
                return l.Pid() < r.Pid();
            }
            return l.getCpu() > r.getCpu(); 
        });
        printf("\033[H");
        printf("\033[J");

        printf("CPU Utilization rate: %.2lf%%\n", s.getCPU());
        printf("Processes: %s | running process: %s | blocked process: %s\n", info[0].c_str(), info[1].c_str(), info[2].c_str());
        for(int i=0;i < std::min((int)procs.size(), 5);i++){
            printf("pid: %s   ppid: %s   status: %s   CPU: %.2lf%%   CMD: %s\n", procs[i].Pid().c_str(),
                                                                                 procs[i].Ppid().c_str(),
                                                                                 procs[i].Status().c_str(),
                                                                                 procs[i].getCpu(),
                                                                                 procs[i].Command().c_str());
        }

        fflush(stdout);
        sleep(1);
    } 
}

int main()
{
    flag = false;
    std::thread collector(Collector);
    std::thread display(Display);

    while(true){
        char choice;
        std::cin >> choice;
        if(choice == 'q'){
            flag = true;
            break;
        }
    }

    collector.join();
    display.join();
    return 0;
}

