cmake_minimum_required(VERSION 3.20)
project(os_experiments LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项: 源码统一使用 UTF-8 编码(含中文注释与输出), 编译器为 MinGW g++
add_compile_options(-Wall -Wextra)

# ---------------------------------------------------------------------------
# 工程结构约定:
#   include/  头文件(数据结构、类接口的定义)
#   src/      与头文件同名的实现文件(.cpp)
#   test/     局部功能测试程序
#   exp/      课程实验程序
#   report/   实验报告(由本人编写)
# ---------------------------------------------------------------------------

# 通用工具库: 终端文本表格
add_library(util STATIC
    src/table.cpp
)
target_include_directories(util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 核心库: 银行家算法
add_library(banker STATIC
    src/banker.cpp
)
target_include_directories(banker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(banker PUBLIC util)

# 课程实验: 死锁避免 —— 银行家算法模拟
add_executable(exp_deadlock
    exp/exp_deadlock.cpp
)
target_link_libraries(exp_deadlock PRIVATE banker)

# 局部测试: 银行家算法
enable_testing()
add_executable(test_banker
    test/test_banker.cpp
)
target_link_libraries(test_banker PRIVATE banker)
add_test(NAME test_banker COMMAND test_banker)

# 局部测试: 表格工具类
add_executable(test_table
    test/test_table.cpp
)
target_link_libraries(test_table PRIVATE util)
add_test(NAME test_table COMMAND test_table)
