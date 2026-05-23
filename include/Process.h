#pragma once

#include "LinuxParser.h"


class Process
{
public:
    Process(std::string& pid):pid_(pid),cpuUtilization_(0) {}
    ~Process() = default;
    std::string Pid()const;//获取pid
    std::string Command()const;//获取命令
    std::string Ppid()const;//获取ppid
    std::string Status()const;//获取进程状态
    bool setInfo();//设置基本信息
    void setCpu(double num);
    double getCpu()const;
    double CpuUtilization();//采样cpu占用
private:
    std::string pid_;
    //进程的基本信息, 命令名, 进程状态，ppid
    std::vector<std::string> procInfo_;
    //CPU占用率
    double cpuUtilization_;
};

