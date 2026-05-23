#pragma once

#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>

//提供静态方法，供外部调用
//关于/proc文件的解析逻辑集中在这里，其他的类不直接触碰文件路径
class LinuxParser
{
public:
    //system info
    static std::string OperatingSystem();   // /etc/os-release 操作系统相关信息
    static std::string Kernel();            // /proc/version   内核相关信息，仅一行

    //CPU,  CPU空闲时间, CPU总时间, 进程数量， 运行进程数量， 阻塞进程数量
    static std::vector<std::string> CpuUtilization(); //读取/proc/stat 返回拆分好的字段,具体字段后续确认

    //Memory
    static double MemoryUtilization();  //返回0 ~ 1 的内存使用比例

    //Uptime, 系统运行时间
    static long long UpTime();   // /proc/uptime
    
    //Process
    static std::vector<std::string> Pids();                 //返回/proc下所有进程的pid
    static double CpuUtilizationOfProcess(std::string& pid);//读取 /proc/[pid]/stat  计算该进程的CPU占用率
    static std::vector<std::string> getProcessInfo(std::string& pid); //得到对应进程的相关信息
};

