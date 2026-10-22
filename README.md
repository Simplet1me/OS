# 操作系统课程实验(OS Experiments)

C++ 实现的操作系统课程实验, 使用 CMake 构建, 编译器为 **MinGW g++**。

## 目录结构

| 目录      | 说明                                       |
| --------- | ------------------------------------------ |
| include/  | 头文件: 数据结构、类接口的定义             |
| src/      | 与头文件同名的实现文件(.cpp)               |
| test/     | 局部功能测试程序                           |
| exp/      | 课程实验程序                               |
| report/   | 实验报告(由本人编写)                       |

## 构建

```bat
build.bat
```

或手动执行:

```bat
cmake --preset mingw
cmake --build --preset mingw-release
```

## 运行

```bat
build\mingw\exp_deadlock.exe    :: 实验: 死锁避免 —— 银行家算法模拟
build\mingw\exp_storage.exe     :: 实验: 基本存储管理 —— 分页式分配与回收(位示图)
build\mingw\test_banker.exe     :: 测试: 银行家算法
build\mingw\test_bitmap.exe     :: 测试: 分页存储管理(位示图)
```

运行测试也可以使用 `ctest --preset mingw-test`。


构建/运行: 

cmake --preset mingw 
cmake --build --preset mingw-release

产物在 build/mingw/

## 实验列表

| 实验 | 内容                             | 源文件                |
| ---- | -------------------------------- | --------------------- |
| 死锁 | 死锁避免 —— 银行家算法模拟       | exp/exp_deadlock.cpp  |
| 基本存储管理 | 分页式内存分配与回收(位示图) | exp/exp_storage.cpp   |

