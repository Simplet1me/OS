# 操作系统课程实验：死锁避免

使用 C++17 和银行家算法模拟资源申请，程序仅通过终端输入输出。

## 工程结构

- `include/banker.h`：资源数据结构、银行家算法接口定义。
- `src/banker.cpp`：同名接口实现，包含安全性检查和资源申请。
- `test/test_banker.cpp`：算法局部测试，通过 CTest 运行。
- `exp/exp_deadlock.cpp`：死锁实验交互程序。
- `report/`：由学生本人编写实验报告，程序不生成或修改报告。

## VS2022 编译与运行

需要 Visual Studio 2022 的“使用 C++ 的桌面开发”组件（v143、Windows SDK）以及 CMake 3.20 或以上。在工程根目录运行：

```powershell
cmake --preset vc2022
cmake --build --preset vc2022
ctest --preset vc2022
.\build\vc2022\Debug\exp_deadlock.exe
```

预设固定使用 `Visual Studio 17 2022`、`v143` 和 `x64`。如果提示找不到 VS 实例，请检查 VS2022 及 C++ 工具是否安装完整。

当前开发环境实际可见的是 Visual Studio 2026 Insiders 的 MSVC 14.51，并未找到 VS2022 实例。本次实际编译验证使用该环境的 x64 Developer Command Prompt：

```text
cmake -S . -B build/msvc-local -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build/msvc-local
ctest --test-dir build/msvc-local --output-on-failure
build\msvc-local\exp_deadlock.exe
```

## 操作说明

启动后选择 `1` 使用经典的 5 进程、3 类资源示例，选择 `2` 自定义进程数量、资源种类数量、Available，以及每个进程的 Max 和 Allocation。

每组数字输入占一行，数字之间用空格分隔。进程从 `P0` 开始编号，资源向量按 `R0 R1 ...` 排列。初始化要求资源数量非负、Allocation 不超过 Max、矩阵维度一致。

随后输入申请进程编号和 Request 向量。支持连续申请；输入 `-2` 查看当前分配表和安全性，输入 `-1` 退出。输入结束（EOF）也会退出。

资源申请依次检查进程编号、资源数量、`Request <= Need` 和 `Request <= Available`，再在副本中试分配并执行安全性算法。找到完整安全序列才提交并打印资源分配表、安全序列，否则提示不能分配并保留原状态。

安全性算法中的资源释放只用于模拟进程完成并判断安全性，不会实际修改分配表。不安全表示无法保证所有进程顺利完成，并不等同于已经发生死锁。

## 示例输入

```text
1
4
3 3 0
1
1 0 2
-1
```

先拒绝 P4 的 `[3 3 0]` 申请（试分配后不安全），再接受 P1 的 `[1 0 2]` 申请。第二次申请后 Available 为 `[2 3 0]`，安全序列为 `P1 -> P3 -> P4 -> P0 -> P2`。

测试覆盖安全序列逐步有效性、安全分配更新、不安全申请不改变状态、超过 Need、可用资源不足、无效输入、连续申请、需要重复扫描的安全状态以及非法初始化。
