#include "Show.h"
#include "Process.h"
#include <ncurses.h>

// ===== 初始化颜色 =====
void init_colors(void)
{
    start_color();
    init_pair(1, COLOR_GREEN,  COLOR_BLACK);   // 正常  绿字
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);   // 警告  黄字
    init_pair(3, COLOR_RED,    COLOR_BLACK);   // 危险  红字
    init_pair(4, COLOR_WHITE,  COLOR_BLUE);    // 标题栏 蓝底白字
    init_pair(5, COLOR_BLACK,  COLOR_CYAN);    // 选中行 黑字白底
    init_pair(6, COLOR_CYAN,   COLOR_BLACK);   // 信息文字 白字
}

//画进度条          窗口   所在行数  所在列   宽度        百分比       颜色编号 
void draw_bar(WINDOW* win, int y, int x, int width, float percent, int color_pair){
    if (width <= 0) return;
    int filled = (int)(width * percent / 100.0f);
    if (filled > width) filled = width;
    
    wattron(win, COLOR_PAIR(color_pair));
    for (int i = 0; i < filled; i++)
        mvwaddch(win, y, x + i, ACS_CKBOARD);  // █ 的替代字符
    wattroff(win, COLOR_PAIR(color_pair));
    
    for (int i = filled; i < width; i++)
        mvwaddch(win, y, x + i, ' ');
}  

//                      窗口     宽度                             系统信息    进程信息    cpu占用
void draw_header(WINDOW *win, int max_x,  std::vector<std::string>& meminfo, 
                 std::string& kernel, std::string& os_release,
 std::vector<std::string>& processInfo, double& cpuInfo, long long& uptime)
{
    // 蓝色标题栏背景
    wattron(win, COLOR_PAIR(4));
    for (int i = 0; i < max_x; i++) mvwaddch(win, 0, i, ' ');
    mvwprintw(win, 0, 2, " System Monitor ");
    wattroff(win, COLOR_PAIR(4));

    // 系统信息
    float cpu_pct = cpuInfo;
    float mem_pct = 0;
    float swp_pct = 0;
    if(meminfo[0] != "N/A"){
        mem_pct = stod(meminfo[2]) * 100;
        swp_pct = stod(meminfo[5]) * 100;
    }
    int bar_w = max_x - 30;
    if (bar_w > 40) bar_w = 40;
    if (bar_w < 5) bar_w = 5;

    //如果大于80%就显示为红色
    int cpu_color = cpu_pct < 50 ? 1 : (cpu_pct < 80 ? 2 : 3);
    int mem_color = mem_pct < 50 ? 1 : (mem_pct < 80 ? 2 : 3);
    int swp_color = swp_pct < 50 ? 1 : (swp_pct < 80 ? 2 : 3);

    mvwprintw(win, 3, 2, "CPU[");
    draw_bar(win, 3, 6, bar_w, cpu_pct, cpu_color);
    mvwprintw(win, 3, 6 + bar_w, "]%5.1f%%", cpu_pct);

    mvwprintw(win, 4, 2, "MEM[");
    draw_bar(win, 4, 6, bar_w, mem_pct, mem_color);
    mvwprintw(win, 4, 6 + bar_w, "]%5.1f%%", mem_pct);

    mvwprintw(win, 5, 2, "SWP[");
    draw_bar(win, 5, 6, bar_w, swp_pct, swp_color);
    mvwprintw(win, 5, 6 + bar_w, "]%5.1f%%", swp_pct);

    wattron(win, COLOR_PAIR(6));
    mvwprintw(win, 1, 2, "System: Linux %s  %s", kernel.c_str(), os_release.c_str());
    mvwprintw(win, 2, 2, "process total: %s  running: %s  sleep: %s", processInfo[0].c_str(), 
                                                                                   processInfo[1].c_str(), 
                                                                                   processInfo[2].c_str());
    mvwprintw(win, 3, 6 + bar_w + 10, "Uptime: %lldday %lldh %lldm", uptime / 86400, uptime % 86400 / 3600, uptime % 3600 / 60);//
    mvwprintw(win, 4, 6 + bar_w + 10, "memory total: %s  available: %s", meminfo[0].c_str(), meminfo[1].c_str());
    mvwprintw(win, 5, 6 + bar_w + 10, "swap total: %s  free: %s", meminfo[3].c_str(), meminfo[4].c_str());
    wattroff(win, COLOR_PAIR(6));
}


// ===== 绘制进程列表    窗口                       进程信息   展示行数  当前选中行   鼠标滚轮偏移  屏幕大小 
void draw_processes(WINDOW *win, std::vector<Process>& procs, int count,
                    int selected, int scroll_offset, int max_y, int max_x)
{
    int visible_lines = max_y - 2;  // 减去上下边框

    // 列标题
    wattron(win, COLOR_PAIR(4));
    for (int i = 0; i < max_x - 1; i++) mvwaddch(win, 0, i, ' ');
    mvwprintw(win, 0, 2, "  PID       PPID  CPU%%    MEM%%  STATUS  COMMAND");
    wattroff(win, COLOR_PAIR(4));

    // 进程行
    int row = 2;  // 窗口内的起始行
    for (int i = 0; i < count && row < max_y; i++) {
        int data_idx = i + scroll_offset;
        if (data_idx >= count) break;

        if (data_idx == selected) {
            // 选中行 - 高亮反色
            wattron(win, COLOR_PAIR(5));
            for (int c = 1; c < max_x - 1; c++) mvwaddch(win, row - 1, c, ' ');
        }

        mvwprintw(win, row - 1, 2, "%5s  %8s  %5.2f   %5.2f    %s    %s",
                  procs[data_idx].Pid().c_str(),
                  procs[data_idx].Ppid().c_str(),
                  procs[data_idx].getCpu(),
                  procs[data_idx].getMem(),
                  procs[data_idx].Status().c_str(),
                  procs[data_idx].Command().c_str());

        if (data_idx == selected) {
            wattroff(win, COLOR_PAIR(5));
        }
        row++;
    }
}


//绘制底部
void draw_footer(WINDOW *win, int max_x)
{
    wattron(win, COLOR_PAIR(4));
    for (int i = 0; i < max_x; i++) mvwaddch(win, 0, i, ' ');
    mvwprintw(win, 0, 1, 
        " k:Kill  q:Quit  s:send signal to process  ↑↓:Navigate ");
    wattroff(win, COLOR_PAIR(4));
    
    wattron(win, COLOR_PAIR(4));
    for (int i = 0; i < max_x; i++) mvwaddch(win, 1, i, ' ');
    mvwprintw(win, 1, 1, " Sort CPU%%: c | PID: p | MEM: m | desc: d ");
    wattroff(win, COLOR_PAIR(4));
}


//绘制信号选中悬浮窗
void draw_signal_menu(WINDOW* win, int signal_sel, int max_y, int max_x){

    //菜单高度，加上上下边框
    int menu_h = signal_options.size() + 2;
    int menu_w = 30;

    //计算当前窗口的中间位置
    int start_y = (max_y - menu_h) / 2;
    int start_x = (max_x - menu_w) / 2;

    //绘制边框
    wattron(win, COLOR_PAIR(4));
    box(win, 0, 0);

    //绘制标题
    mvwprintw(win, start_y, start_x + 2, " Send Signal ");

    //绘制信号的选项
    for(int i=0; i < (int)signal_options.size(); i++){
        if(i == signal_sel){
            wattron(win, COLOR_PAIR(5));//高亮反光
            //覆盖窗口下方的元素
            for(int j=1;j<menu_w -1;j++){
                mvwaddch(win, start_y + 1 + i, start_x + j, ' ');
            }
            mvwprintw(win, start_y+1+i, start_x+2, " > %s", signal_options[i].name.c_str());
            wattroff(win, COLOR_PAIR(5));
        }else{
            wattron(win, COLOR_PAIR(6));
            mvwprintw(win, start_y+1+i, start_x+2, " > %s", signal_options[i].name.c_str());
            wattroff(win, COLOR_PAIR(6));
        }
    }

    wattroff(win, COLOR_PAIR(5));
}
