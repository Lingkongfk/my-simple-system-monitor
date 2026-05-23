#include "LinuxParser.h"
#include <dirent.h>
#include <string>
#include <sys/unistd.h>
#include <fstream>
#include <sstream>
#include <algorithm>
std::string LinuxParser::OperatingSystem(){
    return "/etc/os-release";
}

// System

std::string LinuxParser::Kernel(){
    return "/proc/version";
}

// CPU
std::vector<std::string> LinuxParser::CpuUtilization(){
    std::vector<std::string> ans;
    std::fstream file("/proc/stat", std::ios::in);
    if(!file.is_open()){
        printf("file /proc/stat open failed\n");
        return ans;
    }

    //CPU总运行时间和空闲时间
    std::string str;
    std::string word;
    for(int i=1;i<=11;i++){
        std::getline(file, str);
        if(i == 1){     
            std::stringstream ss(str);
            std::vector<std::string> vec;
            while(ss >> word){
                vec.push_back(word);
            }
            ans.push_back(vec[4]);
            int total = 0;
            for(int i=1;i<vec.size();i++){
                total += stod(vec[i]);
            }
            ans.push_back(std::to_string(total));
        }else if(i == 9){ 
            std::stringstream ss(str);
            std::string word1, word2; 
            ss>>word1 >> word2;
            ans.push_back(word2);
        }else if(i == 10){
            std::stringstream ss(str);
            std::string word1, word2; 
            ss>>word1 >> word2;
            ans.push_back(word2);
        }else if(i == 11){
            std::stringstream ss(str);
            std::string word1, word2; 
            ss>>word1 >> word2;
            ans.push_back(word2);
        }
    }

    file.close();
    return ans;
}

// Memory
double LinuxParser::MemoryUtilization(){
    std::fstream file("/proc/meminfo", std::ios::in);
    if(!file.is_open()){
        printf("file /proc/meminfo open failed\n");
        return 0;
    }

    double ans = 0;

    std::string line;
    std::getline(file, line);
    std::stringstream ss(line);
    std::string word1, word2;
    ss >> word1 >> word2;
    ans += stod(word2);

    for(int i=0;i<5;i++){
        std::getline(file, line);
    }

    std::getline(file, line);
    std::stringstream ss1(line);
    ss >> word1 >> word2;
    ans = (ans / stod(word2));
    return ans;
}

// Uptime
long long LinuxParser::UpTime(){
    std::fstream file("/proc/uptime", std::ios::in);
    if(!file.is_open()){
        printf("file /proc/uptime open failed\n");
        return 0;
    }
    std::string str;
    std::getline(file, str);
    int i=0;
    while(str[i] != ' '){
        i++;
    }
    std::string ans = str.substr(0, i);
    return stod(ans);

}

// Process
std::vector<std::string> LinuxParser::Pids(){
    std::vector<std::string> vec;
    DIR* dir = opendir("/proc");
    if(dir == nullptr){
        printf("open /proc failed\n");
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
    std::string allPath = "/proc/" + pid + "/stat";
    std::string line;

    std::fstream file(allPath, std::ios::in);
    if(!file.is_open()){
        printf("pid:%s stat time info file failed\n", pid.c_str());
        return 0;
    }
    std::getline(file, line);
    double ans = 0;
    std::stringstream ss(line);
    std::string word;
    int cnt = 0;
    while(ss >> word){
        cnt++;
        if(cnt == 14){
            ans += stod(word);
        }else if(cnt == 15){
            ans += stod(word);
        }
    }

    return ans;
}

std::vector<std::string> LinuxParser::getProcessInfo(std::string& pid){

    std::vector<std::string> ans;
    std::string allPath = "/proc/" + pid + "/stat";
    std::string line;

    std::fstream file(allPath, std::ios::in);
    if(!file.is_open()){
        printf("open pid:%s stat file failed\n", pid.c_str());
        return ans;
    }
    std::getline(file, line);
    int s=0, e=0;
    for(int i=0;i<line.size();i++){
        if(line[i] == '('){
            s = i;
        }else if(line[i] == ')'){
            e = i;
            break;
        }
    }
    ans.push_back(line.substr(s+1, e - s - 1));

    file.close();

    allPath = "/proc/" + pid + "/status";

    file.open(allPath, std::ios::in);
    if(!file.is_open()){
        printf("open pid:%s status file failed\n", pid.c_str());
        return ans;
    }

    //如果进程已经结束了
    for(int i=1;i<=7;i++){
        std::getline(file, line);
        if(i == 3){
            //进程状态
            std::stringstream ss(line);
            std::string word1,word2;
            ss >> word1 >> word2;
            ans.push_back(word2);
        }else if(i == 7){
            //进程的ppid
            std::stringstream ss(line);
            std::string word1,word2;
            ss >> word1 >> word2;
            ans.push_back(word2); 
        }
    }
    file.close();
    return ans;

}
