#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "banker.h"

#ifdef _WIN32
#include <windows.h>
#endif

using os::Banker;

namespace {

//三类资源
const std::vector<int> kTotal = {10, 5, 7};
const std::vector<std::vector<int>> kMax = {
    {7, 5, 3},  // P0
    {3, 2, 2},  // P1
    {9, 0, 2},  // P2
    {2, 2, 2},  // P3
    {4, 3, 3},  // P4
};
const std::vector<std::vector<int>> kAlloc = {
    {0, 1, 0},  // P0
    {2, 0, 0},  // P1
    {3, 0, 2},  // P2
    {2, 1, 1},  // P3
    {0, 0, 2},  // P4
};


// // 四类资源 A/B/C/D
// const std::vector<int> kTotal = {10, 5, 7, 6};
// const std::vector<std::vector<int>> kMax = {
//     {7, 5, 3, 2},  // P0
//     {3, 2, 2, 2},  // P1
//     {9, 0, 2, 2},  // P2
//     {2, 2, 2, 1},  // P3
//     {4, 3, 3, 1},  // P4
// };
// const std::vector<std::vector<int>> kAlloc = {
//     {0, 1, 0, 1},  // P0
//     {2, 0, 0, 0},  // P1
//     {3, 0, 2, 2},  // P2
//     {2, 1, 1, 0},  // P3
//     {0, 0, 2, 0},  // P4
// };

// 载入数据
Banker makeExample() {
    return Banker(kTotal, kMax, kAlloc); 
}

/// 读取一个 [lo, hi] 范围内的整数, 非法输入时提示并重试
int readInt(const std::string& prompt, int lo, int hi) {
    for (;;) {
        int v = 0;
        std::cout << prompt;
        if (std::cin >> v && v >= lo && v <= hi) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return v;
        }
        if (std::cin.eof()) {
            std::cout << "\n输入流已结束, 程序退出。\n";
            std::exit(0);
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "输入无效, 请重新输入!\n";
    }
}

/// 读取 m 个非负整数(申请量/需求量等), 非法输入时提示并重试
std::vector<int> readVector(int m, const std::string& prompt) {
    for (;;) {
        std::vector<int> v(m);
        bool ok = true;
        std::cout << prompt;
        for (int j = 0; j < m; ++j) {
            if (!(std::cin >> v[j]) || v[j] < 0) {
                ok = false;
                break;
            }
        }
        if (std::cin.eof()) {
            std::cout << "\n输入流已结束, 程序退出。\n";
            std::exit(0);
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (ok) return v;
        std::cout << "输入无效(需 " << m << " 个非负整数), 请重新输入!\n";
    }
}

/// 资源名提示串, 如 "A B C"
std::string resourceHint(int m) {
    std::ostringstream oss;
    for (int j = 0; j < m; ++j) {
        if (j) oss << " ";
        oss << os::resourceName(j);
    }
    return oss.str();
}

/// 菜单 1: 打印资源分配表与安全性检查过程表(含安全序列结论)
void showState(const Banker& b) {
    b.printTable();
    b.printSafetyTable();
}

/// 菜单 2: 进程申请资源
void doRequest(Banker& b) {
    int pid = readInt("请输入申请资源的进程编号(0~" + std::to_string(b.processCount() - 1) + "): ",
                      0, b.processCount() - 1);
    std::vector<int> req =
        readVector(b.resourceCount(), "请输入进程 " + b.names()[pid] + " 申请各类资源的数量(" +
                                          resourceHint(b.resourceCount()) + "): ");

    std::string reason;
    if (b.request(pid, req, &reason)) {
        std::cout << "\n[申请成功] 系统仍处于安全状态, 同意分配!\n";
        showState(b);  // 打印分配后的资源分配表与安全序列
    } else {
        std::cout << "\n[申请失败] " << reason << ", 不能分配!\n";
    }
}

void printMenu() {
    std::cout << "\n请选择操作:\n"
              << "  1. 显示资源分配表与安全性检查过程\n"
              << "  2. 进程申请资源\n"
              << "  0. 退出\n";
}

}  // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);  // 终端以 UTF-8 显示中文输出
#endif
    Banker b = makeExample();

    // 汇总已载入例题的资源总量(= 各进程已分配总量 + 可用资源), 供欢迎信息显示
    std::ostringstream totals;
    for (int j = 0; j < b.resourceCount(); ++j) {
        if (j) totals << ", ";
        int t = b.available()[j];
        for (int i = 0; i < b.processCount(); ++i) t += b.allocation()[i][j];
        totals << t;
    }

    std::cout << "===============================================\n"
              << "     银行家算法 —— 死锁避免模拟\n"
              << "===============================================\n"
              << "已载入数据: " << b.processCount() << " 个进程、"
              << b.resourceCount() << " 类资源(" << resourceHint(b.resourceCount())
              << ")、总量 (" << totals.str() << ")。\n"
              << "提示:\n"
              << "[1] 查看资源分配表与安全性检查过程;\n"
              << "[2] 令 P1 申请 (1, 0, 2, 2), 观察同意分配后的表格变化;\n"
              << "接着令 P0 申请 (0, 3, 0, 0), 观察因不安全被拒绝的提示。\n";

    for (;;) {
        printMenu();
        switch (readInt("请输入选择: ", 0, 2)) {
            case 1:
                showState(b);
                break;
            case 2:
                doRequest(b);
                break;
            default:  // case 0
                std::cout << "再见!\n";
                return 0;
        }
    }
}
