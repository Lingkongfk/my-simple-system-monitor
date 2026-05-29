#pragma once
#include "Process.h"
#include <ncurses.h>
#include <locale.h>
#include <vector>
#include <string>

//初始话颜色和对应编号
void init_colors();

//画进度条
void draw_bar(WINDOW* win, int y, int x, int width, float percent, int color_pair);

//绘制头部             窗口，    宽度
void draw_header(WINDOW *win, int max_x, std::vector<std::string>& systemInfo,
                 std::vector<std::string>& processInfo, double cpuInfo);

//绘制进程列表
void draw_processes(WINDOW *win, std::vector<Process>& procs, int count,
                    int selected, int scroll_offset, int max_y, int max_x);

//绘制底部
void draw_footer(WINDOW *win, int max_x);
