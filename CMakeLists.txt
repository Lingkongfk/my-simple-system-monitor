cmake_minimum_required(VERSION 3.22.1) #最低版本3.38
project(test) #项目名称test

# 开启 ASan 并生成调试信息
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")


set(CMAKE_CXX_STANDARD 17)#C++编译器版本17
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})#设置输出路径
file(GLOB SRC ${PROJECT_SOURCE_DIR}/src/*.cpp)#打开搜索代码文件
include_directories(${PROJECT_SOURCE_DIR}/include)#头文件路径

add_executable(main ${SRC})

#链接ncurses库
target_link_libraries(main PRIVATE ncursesw)
