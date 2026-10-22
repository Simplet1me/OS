# CLAUDE.md

本仓库是操作系统课程实验工程。编写代码时遵循以下约定:

## 工程结构
- include/ 存放头文件(数据结构、类接口的定义)
- src/ 存放与头文件同名的实现文件(.cpp)
- test/ 存放局部功能测试程序(自包含 main, 不引入第三方测试框架)
- exp/ 存放课程实验程序, 命名 exp_<主题>.cpp
- report/ 存放实验报告

## 构建与编译
- C++17 + CMake, 编译器为 MinGW g++(已在系统 PATH 中, 不使用 VS2022)
- 构建: cmake --preset mingw && cmake --build --preset mingw-release
- 源码统一 UTF-8 编码, 注释与终端输出使用中文
- Windows 终端输出中文前调用 SetConsoleOutputCP(CP_UTF8)

## 输出形式
- 课程实验程序为纯终端文本交互, 不引入图形界面
- 打印表格统一使用通用工具类 Table(include/table.h, src/table.cpp), 不手写对齐逻辑
