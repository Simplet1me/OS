/**
 * @file exp_storage.cpp
 * @brief 实验: 基本存储管理 —— 分页式内存分配与回收(位示图)
 *
 * 在分页式管理方式下, 用位示图表示主存分配情况(0=空闲, 1=已占用),
 * 实现主存空间的分配与回收:
 *   - 装入作业时按页数分配空闲块, 由"块号 = j*8 + i"计算块号并建立页表;
 *   - 作业结束时按页表归还块号, 由"j = 块号/8, i = 块号%8"清空对应位。
 */
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "bitmap.h"

#ifdef _WIN32
#include <windows.h>
#endif

using os::BitMap;

namespace {

// 主存共 64 块, 初始已有 10 块被占用
const int kTotalBlocks = 64;
const std::vector<int> kOccupied = {0, 1, 4, 5, 6, 9, 11, 13, 24, 31};

/// 载入教材例题数据
BitMap makeExample() { return BitMap(kTotalBlocks, kOccupied); }

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

/// 读取若干非负整数(块号列表), 用空格分隔; 非法输入时提示并重试
std::vector<int> readBlockList(const std::string &prompt) {
    for (;;) {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);
        std::istringstream iss(line);
        std::vector<int> v;
        int x;
        bool ok = false;
        while (iss >> x) {
            if (x < 0) {
                ok = false;
                break;
            }
            v.push_back(x);
            ok = true;
        }
        if (ok && iss.eof() && !v.empty()) {
            return v;
        }
        std::cout << "输入无效(需至少一个非负整数, 用空格分隔), 请重新输入!\n";
    }
}

/// 显示位示图与空闲块数
void showState(const BitMap &b) { b.print(); }

/// 菜单 2: 装入作业(分配主存并建立页表)
void doAllocate(BitMap &b) {
    int pages = readInt("请输入作业的页数(需占用的块数): ", 1, b.totalBlocks());
    std::string reason;
    std::vector<int> blocks = b.allocate(pages, &reason);
    if (blocks.empty()) {
        std::cout << "\n[分配失败] " << reason << "\n";
        return;
    }
    std::cout << "\n[分配成功] 已为作业分配 " << blocks.size()
              << " 个块, 建立页表:\n";
    os::printPageTable(blocks);
    std::cout << "分配后的位示图:\n";
    b.print();
}

/// 菜单 3: 回收作业(归还主存块)
void doRecycle(BitMap &b) {
    std::vector<int> blocks =
        readBlockList("请输入作业归还的块号(用空格分隔): ");
    try {
        b.recycle(blocks);
    } catch (const std::invalid_argument &e) {
        std::cout << "\n[回收失败] " << e.what() << "\n";
        return;
    }
    std::cout << "\n[回收成功] 已归还 " << blocks.size() << " 个块: ";
    for (std::size_t k = 0; k < blocks.size(); ++k) {
        if (k) {
            std::cout << ", ";
        }
        std::cout << blocks[k];
    }
    std::cout << "\n回收后的位示图:\n";
    b.print();
}

/// 菜单 4: 运行教材示例(题目 3(5) 的完整场景)
void runDemo() {
    BitMap b = makeExample();
    std::cout << "\n================ 教材示例(初始状态) ================\n";
    b.print();

    std::cout << "\n>>> 步骤 1: 装入信息量为 5 页的作业, 分配主存并建立页表\n";
    std::vector<int> blocks = b.allocate(5);
    os::printPageTable(blocks);
    b.print();

    std::cout << "\n>>> 步骤 2: 另一作业执行结束, 回收块 4、5、6、31\n";
    b.recycle({4, 5, 6, 31});
    b.print();
}

void printMenu() {
    std::cout << "\n请选择操作:\n"
              << "  1. 显示位示图与空闲块数\n"
              << "  2. 装入作业(分配主存并建立页表)\n"
              << "  3. 回收作业(归还主存块)\n"
              << "  4. 运行教材示例(5 页作业分配 + 回收 4/5/6/31 块)\n"
              << "  0. 退出\n";
}

} // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8); // 终端以 UTF-8 显示中文输出
#endif
    BitMap b = makeExample();

    std::cout << "===============================================\n"
              << "     分页存储管理 —— 位示图分配与回收模拟\n"
              << "===============================================\n"
              << "已载入数据: 主存共 " << b.totalBlocks() << " 块, 初始已占用 "
              << b.occupiedCount() << " 块, 空闲 " << b.freeCount() << " 块。\n"
              << "提示:\n"
              << "[1] 查看位示图与空闲块数;\n"
              << "[2] 装入作业(按页数分配块);\n"
              << "[3] 回收作业(归还块号);\n"
              << "[4] 运行教材示例场景;\n";

    for (;;) {
        printMenu();
        switch (readInt("请输入选择: ", 0, 4)) {
        case 1:
            showState(b);
            break;
        case 2:
            doAllocate(b);
            break;
        case 3:
            doRecycle(b);
            break;
        case 4:
            runDemo();
            break;
        default: // case 0
            std::cout << "再见!\n";
            return 0;
        }
    }
}
