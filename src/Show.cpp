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
void draw_header(WINDOW *win, int max_x,  std::vector<std::string>& systemInfo, 
 std::vector<std::string>& processInfo, double cpuInfo)
{
    // 蓝色标题栏背景
    wattron(win, COLOR_PAIR(4));
    for (int i = 0; i < max_x; i++) mvwaddch(win, 0, i, ' ');
    mvwprintw(win, 0, 2, " System Monitor ");
    wattroff(win, COLOR_PAIR(4));

    // 系统信息
    float cpu_pct = cpuInfo;
    float mem_pct = 62.3f;//TODO
    float swp_pct = 12.1f;//TODO
    int bar_w = max_x - 30;
    if (bar_w > 40) bar_w = 40;
    if (bar_w < 5) bar_w = 5;

    //如果大于80%就显示为红色
    int cpu_color = cpu_pct < 50 ? 1 : (cpu_pct < 80 ? 2 : 3);
    int mem_color = mem_pct < 50 ? 1 : (mem_pct < 80 ? 2 : 3);
    int swp_color = swp_pct < 50 ? 1 : (swp_pct < 80 ? 2 : 3);

    mvwprintw(win, 1, 2, "CPU[");
    draw_bar(win, 1, 6, bar_w, cpu_pct, cpu_color);
    mvwprintw(win, 1, 6 + bar_w, "]%5.1f%%", cpu_pct);

    mvwprintw(win, 2, 2, "MEM[");
    draw_bar(win, 2, 6, bar_w, mem_pct, mem_color);
    mvwprintw(win, 2, 6 + bar_w, "]%5.1f%%", mem_pct);

    mvwprintw(win, 3, 2, "SWP[");
    draw_bar(win, 3, 6, bar_w, swp_pct, swp_color);
    mvwprintw(win, 3, 6 + bar_w, "]%5.1f%%", swp_pct);

    wattron(win, COLOR_PAIR(6));
    mvwprintw(win, 1, 6 + bar_w + 10, "System: %s", systemInfo[0].c_str());
    mvwprintw(win, 2, 6 + bar_w + 10, "process total: %s  running: %s  sleep: %s", processInfo[0].c_str(), 
                                                                                   processInfo[1].c_str(), 
                                                                                   processInfo[2].c_str());
    mvwprintw(win, 3, 6 + bar_w + 10, "Uptime: 3d 14h 22m");//TODO
    wattroff(win, COLOR_PAIR(6));
}


// ===== 绘制进程列表    窗口                       进程信息   展示行数  当前选中行   鼠标滚轮偏移  屏幕大小 
void draw_processes(WINDOW *win, std::vector<Process>& procs, int count,
                    int selected, int scroll_offset, int max_y, int max_x)
{
    int visible_lines = max_y - 2;  // 减去上下边框

    // 列标题
    wattron(win, COLOR_PAIR(4));
    for (int i = 1; i < max_x - 1; i++) mvwaddch(win, 1, i, ' ');
    mvwprintw(win, 1, 2, "  PID       PPID  CPU%%  STATUS  COMMAND");
    wattroff(win, COLOR_PAIR(4));

    // 进程行
    int row = 2;  // 窗口内的起始行
    for (int i = 0; i < count && row < max_y; i++) {
        int data_idx = i + scroll_offset;
        if (data_idx >= count) break;

        if (data_idx == selected) {
            // 选中行 - 高亮反色
            wattron(win, COLOR_PAIR(5));
            for (int c = 1; c < max_x - 1; c++) mvwaddch(win, row, c, ' ');
        }

        mvwprintw(win, row, 2, "%5s  %8s  %5.2f   %s  %s",
                  procs[data_idx].Pid().c_str(),
                  procs[data_idx].Ppid().c_str(),
                  procs[data_idx].getCpu(),
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
        " F9:Kill  q:Quit  ↑↓:Navigate ");
    wattroff(win, COLOR_PAIR(4));
    
    wattron(win, COLOR_PAIR(4));
    for (int i = 0; i < max_x; i++) mvwaddch(win, 1, i, ' ');
    mvwprintw(win, 1, 1, " Sort: CPU%% | PID  (press s to change sort)");
    wattroff(win, COLOR_PAIR(4));
}
