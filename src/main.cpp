#include <chrono>
#include <iostream>
#include "LinuxParser.h"
#include "Process.h"
#include "System.h"
#include <algorithm>
#include <mutex>
#include <ncurses.h>
#include <sys/select.h>
#include <sys/unistd.h>
#include <thread>
#include <atomic>
#include <vector>
#include <sys/types.h>
#include <signal.h>
#include "Show.h"


//声明全局变量
System s;
std::atomic_bool flag;//判断是否退出

enum class SORT_BY{
    BY_CPU,
    BY_PID,
    BY_MEM
};


//实现滚轮滑动窗口  //总数量，    视野显示数量， 当前选中， 偏移
void adjust_scroll(int total_items, int visible_lines, 
                   int *selected, int *scroll_offset)
{
    // 1. 边界安全限制
    if (*selected < 0) *selected = 0;
    if (*selected >= total_items) *selected = total_items - 1;

    // 2. 如果选中行跑到视口上方去了，把视口往上拉
    if (*selected < *scroll_offset) {
        *scroll_offset = *selected;
    }

    // 3. 如果选中行跑到视口下方去了，把视口往下拉
    if (*selected >= *scroll_offset + visible_lines) {
        *scroll_offset = *selected - visible_lines + 1;
    }

    // 4. 视口本身的边界限制
    if (*scroll_offset < 0) *scroll_offset = 0;
    if (total_items > visible_lines) {
        if (*scroll_offset > total_items - visible_lines) {
            *scroll_offset = total_items - visible_lines;
        }
    } else {
        *scroll_offset = 0; // 数据不够一屏，偏移量必须是0
    }
}


//信息采集线程，每隔1s调用Update()，更新数据
void Collector(){
    while(true){
        if(flag){
            break;
        }
        s.Update();
    }
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
        return false;
    };

    if(desc){
        sort(procs.begin(), procs.end(), [now_by, cmpString](const Process& l, const Process& r)->bool{
            if(now_by == SORT_BY::BY_CPU) return l.getCpu() > r.getCpu();
            else if(now_by == SORT_BY::BY_PID) return cmpString(r.Pid(), l.Pid()); 
            else if(now_by == SORT_BY::BY_MEM) return l.getMem() > r.getMem(); 
            else return false;
        });
    }else{
        sort(procs.begin(), procs.end(), [now_by, cmpString](const Process& l, const Process& r)->bool{
            if(now_by == SORT_BY::BY_CPU) return l.getCpu() < r.getCpu();
            else if(now_by == SORT_BY::BY_PID) return cmpString(l.Pid(), r.Pid()); 
            else if(now_by == SORT_BY::BY_MEM) return l.getMem() < r.getMem(); 
            else return false;
        });
    }
}


//UI线程，每隔200ms读取一次数据并且刷新显示
void Display(){

    //设置系统默认编码
    setlocale(LC_ALL, "");
    //初始化窗口
    initscr();
    //按键设置
    cbreak();//立即接收按键，不等回车
    noecho();//用户按键不显示在屏幕上
    keypad(stdscr, TRUE);//启用功能键 F1 - F12 方向键
    curs_set(0);//隐藏光标
    init_colors();//初始化颜色编号

    //获取屏幕行数列数
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    //创建三个窗口
    WINDOW* win_header = newwin(6, max_x, 0, 0);//最上面5行
    WINDOW* win_body= newwin(max_y - 9, max_x, 6, 0);//中间所有
    WINDOW* win_footer= newwin(3, max_x, max_y - 3, 0);//最下面三行

    int selected = 0;//当前选择行
    int running = 1; //是否正在运行
    int scroll_offset = 0;//滚动页面偏移

    int signal_sel = 0;//信号选中光标
    AppState state = AppState::NORMAL; 

    //排序状态
    bool desc = false; // 降序是false
    SORT_BY now_by = SORT_BY::BY_PID;

    halfdelay(5);//按键等待0.5s

    refresh();//刷新整个页面，这样下面的窗口刷新才能生效
    while(running){
        //清空窗口
        werase(win_header);
        werase(win_body);
        werase(win_footer);
    
        //获取数据
        s.lock(); 
        std::vector<std::string> meminfo = std::move(s).meminfo();
        std::string kernel = std::move(s.Kernel());
        std::string os_release = std::move(s.os_release());
        std::vector<std::string> info = std::move(s).Utilization();//进程数，运行数，阻塞数
        std::vector<Process>& procs = s.Processes();//进程数组, pid ppid status CPU cmd 
        double CPU_utili = s.getCPU();//s.getCPU()可以获得总统CPU使用率
        s.unlock();
        long long uptime = LinuxParser::UpTime();

        //按照cpu排序
        sortByField(procs, now_by, desc);
        
        //对数据进程处理
        if(info.size() < 3){
            info.resize(3, "N/A");
        }
        if(meminfo.size() < 6){
            meminfo.resize(6, "N/A");
        }

        //利用获取的数据进行绘制
        draw_header(win_header, max_x, meminfo, kernel, os_release, info, CPU_utili, uptime);
        draw_processes(win_body, procs, procs.size(), selected, scroll_offset, max_y - 8, max_x);
        draw_footer(win_footer, max_x);

        //如果是信号发送状态，要绘制信号窗口
        if(state == AppState::SEND_SIGNAL){
            draw_signal_menu(win_body, signal_sel, max_y, max_x);
        }

        //刷新所有窗口
        wrefresh(win_header);
        wrefresh(win_body);
        wrefresh(win_footer);

        int ch = getch();

        //判断当前状态        
        if(state == AppState::NORMAL){
            switch(ch){
            case 'q':
                running = 0;
                break;
            case KEY_UP:
                selected--;
                adjust_scroll(procs.size(), max_y - 8 - 2, &selected, &scroll_offset);
                break;
            case KEY_DOWN:
                selected++;
                adjust_scroll(procs.size(), max_y - 8 - 2, &selected, &scroll_offset);
                break;
            case 'k':
                kill(stoi(procs[selected].Pid()), SIGKILL);
                break;
            case 's':
                state = AppState::SEND_SIGNAL;
                signal_sel = 0;
                break;
            case 'c':
                now_by = SORT_BY::BY_CPU;
                break;
            case 'p':
                now_by = SORT_BY::BY_PID;
                break;
            case 'm':
                now_by = SORT_BY::BY_MEM;
                break;
            case 'd':
                if(desc){
                    desc = false;
                }else{
                    desc = true;
                }
                break;
            case ERR:
                break;
            }
        }else if(state == AppState::SEND_SIGNAL){
            switch(ch){
            case KEY_UP:
                signal_sel--;
                if(signal_sel < 0) signal_sel = 0;
                break;
            case KEY_DOWN:
                signal_sel++;
                if(signal_sel >= (int)signal_options.size()) signal_sel = signal_options.size() - 1;
                break;
            case 10: //这个是回车键
                if(!procs.empty() && selected < procs.size()){
                    kill(stoi(procs[selected].Pid()), signal_options[signal_sel].sig);
                    state = AppState::NORMAL;
                }
                break;
            case 'q':
                state = AppState::NORMAL;
                break;
            }
        }


    }

    //清理
    delwin(win_header);
    delwin(win_body);
    delwin(win_footer);
    endwin();
}

int main()
{
    flag = false;
    std::thread collector(Collector);

    Display();
    flag = true;
    collector.join();
    return 0;
}

