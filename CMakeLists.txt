cmake_minimum_required(VERSION 3.22.1) #最低版本3.38
project(test) #项目名称test

set(CMAKE_CXX_STANDARD 17)#C++编译器版本17
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})#设置输出路径
file(GLOB SRC ${PROJECT_SOURCE_DIR}/src/*.cpp)#打开搜索代码文件
include_directories(${PROJECT_SOURCE_DIR}/include)#头文件路径

add_executable(main ${SRC})
