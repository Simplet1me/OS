/**
 * @file exp_page_replacement.cpp
 * @brief 实验: 页面置换算法 —— FIFO 与 LRU 的模拟与对比
 *
 * 模拟实现 FIFO(先进先出)与 LRU(最近最少使用)两种页面置换算法, 统计并对比
 * 缺页次数、缺页率、命中次数、命中率、置换次数等性能指标。两种算法放在同一模块
 * 中, 可在同一份访问序列(副本)上分别独立运行以对比优劣。
 */
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "page_replacement.h"
#include "table.h"

#ifdef _WIN32
#include <windows.h>
#endif

using os::Algorithm;
using os::PageReplacement;
using os::PageResult;
using os::Table;

namespace {

// 默认参数
const int kDefaultPageCount = 10; // 进程页面数 PP
const int kDefaultFrameCount = 3; // 分配页框数 AP
const int kDefaultTotal = 20;     // 访问序列长度

// 经典例题(教材): 8 页、3 页框, FIFO 缺页 15 次, LRU 缺页 12 次
const int kClassicPageCount = 8;
const int kClassicFrameCount = 3;
const std::vector<int> kClassicRefs = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3,
                                       0, 3, 2, 1, 2, 0, 1, 7, 0, 1};

int g_pageCount = kDefaultPageCount;
int g_frameCount = kDefaultFrameCount;
std::vector<int> g_refs;

/// 读取一个 [lo, hi] 范围内的整数, 非法输入时提示并重试
int readInt(const std::string &prompt, int lo, int hi) {
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

/// 读取 total 个页号(取值 0~pageCount-1), 非法输入时提示并重试
std::vector<int> readRefs(int total, int pageCount) {
    for (;;) {
        std::vector<int> v(static_cast<std::size_t>(total));
        bool ok = true;
        std::cout << "请依次输入 " << total << " 个页面号(取值 0~"
                  << pageCount - 1 << ", 用空格分隔): ";
        for (int i = 0; i < total; ++i) {
            if (!(std::cin >> v[static_cast<std::size_t>(i)]) ||
                v[static_cast<std::size_t>(i)] < 0 ||
                v[static_cast<std::size_t>(i)] >= pageCount) {
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
        if (ok) {
            return v;
        }
        std::cout << "输入无效, 请重新输入!\n";
    }
}

/// 打印访问序列(超长时截断显示)
void printRefs(const std::vector<int> &refs) {
    const std::size_t limit = 40;
    for (std::size_t i = 0; i < refs.size() && i < limit; ++i) {
        if (i) {
            std::cout << ", ";
        }
        std::cout << refs[i];
    }
    if (refs.size() > limit) {
        std::cout << ", ... (共 " << refs.size() << " 项)";
    }
    std::cout << "\n";
}

const char *algoName(Algorithm a) {
    return a == Algorithm::FIFO ? "FIFO" : "LRU";
}

/// 打印单一算法的性能指标
void printMetrics(const std::string &name, const PageResult &r) {
    Table table({"指标", "数值"});
    table.setTitle(name + " 性能指标");
    table.addRow({"访问总次数", std::to_string(r.total)});
    table.addRow({"缺页次数", std::to_string(r.faults)});
    table.addRow({"缺页率", os::formatPercent(r.faultRate())});
    table.addRow({"命中次数", std::to_string(r.hits())});
    table.addRow({"命中率", os::formatPercent(r.hitRate())});
    table.addRow({"置换次数", std::to_string(r.replacements)});
    table.print();
}

/// 菜单 1: 设置参数并生成/输入访问序列
void setupConfig() {
    g_pageCount = readInt("请输入进程页面数 PP: ", 1, 100000);
    g_frameCount = readInt("请输入分配的内存页框数 AP: ", 1, 100000);
    if (g_frameCount >= g_pageCount) {
        std::cout
            << "[提示] AP >= PP 时进程可整体驻留, 缺页仅发生在前几次装入。\n";
    }
    int total = readInt("请输入页面访问序列长度: ", 1, 1000000);

    int mode = readInt("访问序列来源: [1] 随机生成  [2] 手动输入: ", 1, 2);
    if (mode == 1) {
        unsigned seed = static_cast<unsigned>(std::random_device{}());
        g_refs = os::randomRefs(total, g_pageCount, seed);
        std::cout << "已随机生成序列(种子 " << seed << "): ";
        printRefs(g_refs);
    } else {
        g_refs = readRefs(total, g_pageCount);
    }
    std::cout << "参数设置完成: PP=" << g_pageCount << ", AP=" << g_frameCount
              << ", 序列长度=" << g_refs.size() << "。\n";
}

/// 运行单个算法
void runOne(Algorithm algo) {
    if (g_refs.empty()) {
        std::cout << "请先设置参数并生成访问序列(菜单 1)。\n";
        return;
    }
    PageReplacement pr(g_frameCount, g_refs);
    PageResult r = pr.run(algo);
    if (g_refs.size() <= 32) {
        os::printTrace(r, g_frameCount,
                       std::string(algoName(algo)) + " 页面置换过程 (AP=" +
                           std::to_string(g_frameCount) + ")");
    } else {
        std::cout << "[提示] 序列较长, 已省略逐步过程表, 仅显示性能指标。\n";
    }
    printMetrics(algoName(algo), r);
}

/// 菜单 4: 两种算法同时运行(同一份访问序列的副本)并对比
void runCompare() {
    if (g_refs.empty()) {
        std::cout << "请先设置参数并生成访问序列(菜单 1)。\n";
        return;
    }
    PageReplacement pr(g_frameCount, g_refs);
    PageResult fifo = pr.runFIFO();
    PageResult lru = pr.runLRU();

    if (g_refs.size() <= 32) {
        os::printTrace(fifo, g_frameCount,
                       "FIFO 页面置换过程 (AP=" + std::to_string(g_frameCount) +
                           ")");
        os::printTrace(lru, g_frameCount,
                       "LRU 页面置换过程 (AP=" + std::to_string(g_frameCount) +
                           ")");
    } else {
        std::cout << "[提示] 序列较长, 已省略逐步过程表, 仅显示性能对比。\n";
    }
    os::printComparison(fifo, lru);

    if (lru.faults < fifo.faults) {
        std::cout << "结论: LRU 缺页更少, 本次表现更优。\n";
    } else if (lru.faults > fifo.faults) {
        std::cout << "结论: FIFO 缺页更少, 本次表现更优。\n";
    } else {
        std::cout << "结论: 两种算法缺页次数相同。\n";
    }
}

/// 菜单 5: 载入并运行经典例题
void runClassic() {
    g_pageCount = kClassicPageCount;
    g_frameCount = kClassicFrameCount;
    g_refs = kClassicRefs;
    std::cout << "已载入经典例题: PP=" << g_pageCount << ", AP=" << g_frameCount
              << ", 访问序列:\n";
    printRefs(g_refs);
    runCompare();
}

void printMenu() {
    std::cout << "\n请选择操作:\n"
              << "  1. 设置参数并生成访问序列\n"
              << "  2. 运行 FIFO 算法\n"
              << "  3. 运行 LRU 算法\n"
              << "  4. 同时运行 FIFO 与 LRU 并对比\n"
              << "  5. 运行经典例题(8 页, 3 页框)\n"
              << "  0. 退出\n";
}

} // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8); // 终端以 UTF-8 显示中文输出
#endif
    std::cout << "===============================================\n"
              << "     页面置换算法 —— FIFO 与 LRU 模拟\n"
              << "===============================================\n"
              << "当前参数: PP=" << g_pageCount << ", AP=" << g_frameCount
              << "。请先通过菜单 1 生成访问序列。\n";

    for (;;) {
        printMenu();
        switch (readInt("请输入选择: ", 0, 5)) {
        case 1:
            setupConfig();
            break;
        case 2:
            runOne(Algorithm::FIFO);
            break;
        case 3:
            runOne(Algorithm::LRU);
            break;
        case 4:
            runCompare();
            break;
        case 5:
            runClassic();
            break;
        default: // case 0
            std::cout << "再见!\n";
            return 0;
        }
    }
}
