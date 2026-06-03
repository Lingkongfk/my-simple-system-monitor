#pragma once
#include "Process.h"
#include <ncurses.h>
#include <locale.h>
#include <vector>
#include <string>


enum class AppState{
    NORMAL,     //正常显示进程列表
    SEND_SIGNAL //选中发送信信号状态
};

//预定义一些信号的列表
struct SignalOption{
    int sig;
    std::string name;
};
const std::vector<SignalOption> signal_options = {
    {9,  "SIGKILL(9) - 强制杀死"},
    {15, "SIGTERM(15) - 优雅终止"},
    {2,  "SIGINT(2) - 中断(ctrl+c)"},
    {19, "SIGSTOP(19) - 暂停"},
    {18, "SIGCONT(18) - 继续"}
};


//初始话颜色和对应编号
void init_colors();

//画进度条
void draw_bar(WINDOW* win, int y, int x, int width, float percent, int color_pair);

//绘制头部             窗口，    宽度
void draw_header(WINDOW *win, int max_x, std::vector<std::string>& meminfo,
                 std::string kernel, std::string os_release,
                 std::vector<std::string>& processInfo, double cpuInfo, long long uptime);

//绘制进程列表
void draw_processes(WINDOW *win, std::vector<Process>& procs, int count,
                    int selected, int scroll_offset, int max_y, int max_x);

//绘制底部
void draw_footer(WINDOW *win, int max_x);

//绘制悬浮窗
void draw_signal_menu(WINDOW* win, int signal_sel, int max_y, int max_x);
