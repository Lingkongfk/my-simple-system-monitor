#include <chrono>
#include <iostream>
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
#include "Show.h"


//声明全局变量
System s;
std::atomic_bool flag;//判断是否退出

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
        std::this_thread::sleep_for(std::chrono::seconds(1));
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
    WINDOW* win_header = newwin(5, max_x, 0, 0);//最上面5行
    WINDOW* win_body= newwin(max_y - 8, max_x, 5, 0);//中间所有
    WINDOW* win_footer= newwin(3, max_x, max_y - 3, 0);//最下面三行

    int selected = 0;//当前选择行
    int running = 1; //是否正在运行
    int scroll_offset = 0; 
    halfdelay(5);//按键等待0.5s
    
    refresh();//刷新整个页面，这样下面的窗口刷新才能生效
    while(running){
        //清空窗口
        werase(win_header);
        werase(win_body);
        werase(win_footer);

        //获取数据
        std::vector<std::string> systemInfo = {"Linux Ubuntu22.04"}; //模拟系统数据TODO
        std::vector<std::string> info = s.Utilization();//进程数，运行数，阻塞数
        std::vector<Process> procs = s.Processes();//进程数组, pid ppid status CPU cmd 
        //s.getCPU()可以获得总统CPU使用率
        double CPU_utili = s.getCPU();

        //利用获取的数据进行绘制
        draw_header(win_header, max_x, systemInfo, info, CPU_utili);
        draw_processes(win_body, procs, procs.size(), selected, scroll_offset, max_y - 8, max_x);
        draw_footer(win_footer, max_x);

        //刷新所有窗口
        wrefresh(win_header);
        wrefresh(win_body);
        wrefresh(win_footer);

        int ch = getch();
        switch(ch){
        case 'q':
            running = 0;
            break;
        case KEY_UP:
            selected--;
            adjust_scroll(procs.size(), max_y - 8, &selected, &scroll_offset);
            break;
        case KEY_DOWN:
            selected++;
            adjust_scroll(procs.size(), max_y - 8, &selected, &scroll_offset);
            break;
        case 'k':
            //TODO
            break;
        case ERR:
            break;
        }
    }

    //清理
    delwin(win_header);
    delwin(win_body);
    delwin(win_footer);
    endwin();
#if 0
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
#endif
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

