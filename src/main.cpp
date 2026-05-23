#include <iostream>
#include "Process.h"
#include "System.h"
#include <algorithm>
#include <sys/unistd.h>

int main()
{
    System s;
    while(true){
        s.Update();

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



    return 0;
}

