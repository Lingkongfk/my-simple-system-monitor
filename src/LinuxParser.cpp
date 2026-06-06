#include "LinuxParser.h"
#include <dirent.h>
#include <fcntl.h>
#include <string>
#include <sys/unistd.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>

//日志是否打开
const bool flag = false;

//返回发行版信息
std::string LinuxParser::OperatingSystem(){
    int file_fd = open("/etc/os-release",O_RDONLY);
    if(file_fd < 0){
        if(flag)
            fprintf(stderr, "file /etc/os-release open failed\n");
        return "";
    }

    char buf[1024] = {0};
    int ret = read(file_fd, buf, sizeof(buf));
    if(ret <= 0){
        if(flag)
            fprintf(stderr, "file /etc/os-release read failed\n");
        return "";
    }

    close(file_fd);

    int l = 0;
    int r = 0;
    for(int i=0;i<sizeof(buf);i++){
        if(buf[i] == '"' && l == 0){
            l = i;
        }else if(buf[i] == '"'){
            r = i;
            break;
        }
    }
    return std::string(buf + l + 1 , buf + r);
}

// System
//返回内核版本信息
std::string LinuxParser::Kernel(){
    int file_fd = open("/proc/version" ,O_RDONLY);
    if(file_fd < 0){
        if(flag)
            fprintf(stderr, "file /proc/version open failed\n");
        return "";
    }

    char buf[1024] = {0};
    int ret = read(file_fd, buf, sizeof(buf));
    if(ret <= 0){
        if(flag)
            fprintf(stderr, "file /proc/version read failed\n");
        return "";
    }

    close(file_fd);
    
    int l = 0;
    int r = 0;
    int cnt = 0;
    for(int i=0;i<sizeof(buf);i++){
        if(buf[i] == ' ' && l == 0){
            l = i;
        }else if(buf[i] == ' '){
            if(cnt == 1){
                r = i;
                break;
            }
            cnt++;
        }
    }
    return std::string(buf + l + 1 , buf + r);
}

// CPU
std::vector<std::string> LinuxParser::CpuUtilization(){
    std::vector<std::string> ans;
    char buf[4096]; // 栈上分配 4KB，足够装下 /proc/stat
    int fd = open("/proc/stat", O_RDONLY);
    if (fd < 0) return ans;

    ssize_t len = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (len <= 0) return ans;
    
    buf[len] = '\0'; // 加上结束符

    char *p = buf;
    char *line_end;

    // 逐行扫描
    while (*p) {
        line_end = strchr(p, '\n');//找到第一行结尾
        if (line_end) *line_end = '\0'; // 将换行符替换为 \0，方便字符串处理

        // 1. 解析 CPU 累计时间：以 "cpu " 开头（注意有空格，区分 cpu0）
        // 格式: cpu  user nice system idle iowait irq softirq steal guest guest_nice
        if (strncmp(p, "cpu ", 4) == 0) {
            p += 4; // 跳过 "cpu "
            
            unsigned long long total = 0;
            unsigned long long idle = 0;
            int field_index = 0;
            char *next_val;

            // 循环提取这一行所有的数字
            while (*p) {
                while (*p == ' ') p++; // 跳过空格
                if (*p == '\0') break;

                unsigned long long val = strtoull(p, &next_val, 10);
                if (p == next_val) break; // 没有数字了

                total += val; // 所有字段累加就是总时间
                
                // idle 是第 4 个字段 (索引 3)，iowait 是第 5 个字段 (索引 4)
                if (field_index == 3) idle = val;
                if (field_index == 4) idle += val; // 空闲时间 = idle + iowait

                p = next_val;
                field_index++;
            }
            
            ans.push_back(std::to_string(idle));
            ans.push_back(std::to_string(total));
        }
        
        // 2. 解析进程数量
        //processes 123456
        else if (strncmp(p, "processes ", 10) == 0) {
            int processes = strtoul(p + 10, NULL, 10);
            ans.push_back(std::to_string(processes));
        }
        
        // 3. 解析运行进程数
        //procs_running 2
        else if (strncmp(p, "procs_running ", 14) == 0) {
            int procs_running = strtoul(p + 14, NULL, 10);
            ans.push_back(std::to_string(procs_running));
        }
        
        // 4. 解析阻塞进程数
        //procs_blocked 1
        else if (strncmp(p, "procs_blocked ", 14) == 0) {
            int procs_blocked = strtoul(p + 14, NULL, 10);
            ans.push_back(std::to_string(procs_blocked));
        }

        // 移动到下一行
        if (line_end) {
            p = line_end + 1; 
        } else {
            break; // 最后一行
        }
    }
    return ans;
}

// Memory 总内存，可使用内存，内存使用率， 总交换内存， 空闲交换内存， swap分区占用率
std::vector<std::string> LinuxParser::MemoryUtilization(){
    std::vector<std::string> ans;
    char buf[4096]; // 栈上分配 4KB，足够装下 /proc/stat
    int fd = open("/proc/meminfo", O_RDONLY);
    if (fd < 0) return ans;

    ssize_t len = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (len <= 0) return ans;
    
    buf[len] = '\0'; // 加上结束符

    char *p = buf;
    char *line_end;
    long long memTotal = 100000000000;
    long long swapTotal = 100000000000;

    // 逐行扫描
    while (*p) {
        line_end = strchr(p, '\n');//找到第一行结尾
        if (line_end) *line_end = '\0'; // 将换行符替换为 \0，方便字符串处理

        //memtotal 12312312 KB
        if (strncmp(p, "MemTotal: ", 10) == 0) {
            //提取该字符串里面的数字和字符串，最后是转换的进制
            memTotal = strtoull(p+10, NULL, 10);
            ans.push_back(std::to_string(memTotal));
        }
        else if (strncmp(p, "MemAvailable: ", 14) == 0) {
            long long memFree = strtoul(p + 14, NULL, 10);
            ans.push_back(std::to_string(memFree));

            double tmp = (memTotal - memFree)*1.0 / memTotal;
            ans.push_back(std::to_string(tmp));
        }
        else if (strncmp(p, "SwapTotal: ", 11) == 0) {
            swapTotal = strtoul(p + 11, NULL, 10);
            ans.push_back(std::to_string(swapTotal));
        }
        else if (strncmp(p, "SwapFree: ", 10) == 0) {
            long long swapFree = strtoul(p + 14, NULL, 10);
            ans.push_back(std::to_string(swapFree));

            double tmp =((swapTotal - swapFree)*1.0) / swapTotal;
            ans.push_back(std::to_string(tmp));
        }

        // 移动到下一行
        if (line_end) {
            p = line_end + 1; 
        } else {
            break; // 最后一行
        }
    }

    return ans;
}

// Uptime
long long LinuxParser::UpTime(){
    int file_fd = open("/proc/uptime" ,O_RDONLY);
    if(file_fd < 0){
        if(flag)
            fprintf(stderr, "file /proc/uptime open failed\n");
        return 0;
    }

    char buf[1024] = {0};
    int ret = read(file_fd, buf, sizeof(buf));
    if(ret <= 0){
        if(flag)
            fprintf(stderr, "file /proc/uptime read failed\n");
        return 0;
    }

    close(file_fd);
    
    long long ans = 0;
    for(int i=0;i<sizeof(buf);i++){
        if(buf[i] == '.' || buf[i] == 0){
            break;
        }
        ans = ans*10 + (buf[i] - '0');
    }
    return  ans;

}

// Process
std::vector<std::string> LinuxParser::Pids(){
    std::vector<std::string> vec;
    DIR* dir = opendir("/proc");
    if(dir == nullptr){
        if(flag) 
            fprintf(stderr,"open /proc failed\n");
        return vec;
    }

    //找到所有pid
    struct dirent* info;
    while((info = readdir(dir)) != nullptr){
        if(info->d_type == DT_DIR){
            //判断，如果目录名字为数字，那么就是进程pid
            std::string name(info->d_name);
            if (!name.empty() && std::all_of(name.begin(), name.end(), ::isdigit)) {
                vec.push_back(name);
            }
        }
    }
    closedir(dir);
    return vec;
}



double LinuxParser::CpuUtilizationOfProcess(std::string& pid){
    std::string path = "/proc/" + pid + "/stat";
    char buf[1024] = {0};
    int fd = open(path.c_str(), O_RDONLY);
    if(fd <= 0){
        return 0;
    }

    ssize_t len = read(fd, buf, sizeof(buf));
    close(fd);
    if(len <= 0) return 0;

    buf[len] = '\0';
    int i=0;
    while(i < len){
        if(buf[i] == ')'){
            break;
        }
        i++;
    }
    while(i < len){
        if(buf[i] >= 'A' && buf[i] <= 'Z'){
            break;
        }
        i++;
    }
    
    char* p = buf + i + 1;
    char *next_val;
    int cnt = 4;
    double ans = 0;
    while(*p){
        while(*p == ' ') p++; //跳过空格
        if(*p == 0) break;
        
        long long tmp = strtoull(p, &next_val, 10);
        if(p == next_val) break; //没有数字了
        if(cnt == 14){
            ans += (double)tmp;
        }else if(cnt == 15){
            ans += (double)tmp;
        }else if(cnt > 16){
            break;
        }
        p = next_val;
        cnt++;
    }
    return ans;
}

double LinuxParser::MemUtilizationOfProcess(std::string& pid){
    std::string path = "/proc/" + pid + "/stat";
    char buf[1024] = {0};
    int fd = open(path.c_str(), O_RDONLY);
    if(fd <= 0){
        return 0;
    }

    ssize_t len = read(fd, buf, sizeof(buf));
    close(fd);
    if(len <= 0) return 0;

    buf[len] = '\0';
    int i=0;
    while(i < len){
        if(buf[i] == ')'){
            break;
        }
        i++;
    }
    while(i < len){
        if(buf[i] >= 'A' && buf[i] <= 'Z'){
            break;
        }
        i++;
    }
    
    char* p = buf + i + 1;
    char *next_val;
    int cnt = 4;
    double ans = 0;
    while(*p){
        while(*p == ' ') p++; //跳过空格
        if(*p == 0) break;
        
        long long tmp = strtoull(p, &next_val, 10);
        if(p == next_val) break; //没有数字了
        if(cnt == 24){
            ans += (double)tmp;
        }else if(cnt > 25){
            break;
        }
        p = next_val;
        cnt++;
    }
    return ans;
}

std::vector<std::string> LinuxParser::getProcessInfo(std::string& pid){

    std::vector<std::string> ans;

    std::string path = "/proc/" + pid + "/stat";
    char buf[1024] = {0};
    int fd = open(path.c_str(), O_RDONLY);
    if(fd <= 0){
        return ans;
    }

    ssize_t len = read(fd, buf, sizeof(buf));
    close(fd);
    if(len <= 0) return ans;

    buf[len] = 0;
    int l = 0;
    int r = 0;
    for(int i=0;i<sizeof(buf);i++){
        if(buf[i] == '(' && l == 0){
            l = i;
        }else if(buf[i] == ')'){
            r = i;
            break;
        }
    }
    //进程命令
    ans.push_back(std::string(buf+l+1, buf+r)); 

    //状态
    char* p = buf + r + 1;
    while(*p == ' ')p++;
    if(*p >= 'A' && *p <= 'Z'){
        ans.push_back(std::string(p, p+1));
        p++;
    }

    while(*p == ' ')p++;
    int next = 0;
    while(*(p+next) >= '0' && *(p+next) <= '9'){
        next++;
    }
    ans.push_back(std::string(p, p+next));


    return ans;

}
