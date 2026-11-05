/**
 * @file exp_disk.cpp
 * @brief 实验: 磁盘存储空间的分配与回收(位示图)
 *
 * 用位示图记录磁盘空间使用情况, 实现磁盘块的分配与回收:
 *   - 申请磁盘块时查位示图找空闲位, 由"相对块号 = 字号×16+位号"算相对块号,
 *     再换算物理地址(柱面号/盘面号/扇区号), 并建立"用户已获得盘块情况表"。
 *   - 回收磁盘块时由物理地址算相对块号, 再算字号/位号, 将对应位清 0。
 * 要求显示分配/回收前后的位示图、空闲块数, 以及分配块的物理地址。
 */
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "disk_bitmap.h"

#ifdef _WIN32
#include <windows.h>
#endif

using os::DiskAddress;
using os::DiskBitmap;
using os::DiskBlock;
using os::DiskGeometry;

namespace {

// 教材例题: 40 柱面 × 4 盘面 × 8 扇区 = 1280 块, 字长 16 位
const DiskGeometry kGeometry = {40, 4, 8, 16};
// 初始已占用的相对块号(分散在不同柱面, 便于观察地址换算)
const std::vector<int> kOccupied = {0, 1, 15, 16, 17, 31, 32, 63, 64, 1279};

std::vector<DiskBlock> g_lastAllocated; // 最近一次分配得到的块

/// 载入教材例题数据
DiskBitmap makeExample() { return DiskBitmap(kGeometry, kOccupied); }

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

/// 读取若干物理地址(每块 3 个数: 柱面号 盘面号 扇区号), 非法输入时提示并重试
std::vector<DiskAddress> readAddresses() {
    for (;;) {
        std::cout << "请输入要回收的物理地址(每块 3 个数: 柱面号 盘面号 "
                     "扇区号, 空格分隔): ";
        std::string line;
        std::getline(std::cin, line);
        std::istringstream iss(line);
        std::vector<int> nums;
        int x;
        while (iss >> x) {
            nums.push_back(x);
        }
        if (nums.size() % 3 != 0 || nums.empty()) {
            std::cout << "输入无效: 需输入 3 的整数倍个整数(每块 = 柱面号 "
                         "盘面号 扇区号)。\n";
            continue;
        }
        std::vector<DiskAddress> addrs;
        addrs.reserve(nums.size() / 3);
        for (std::size_t i = 0; i < nums.size(); i += 3) {
            addrs.push_back({nums[i], nums[i + 1], nums[i + 2]});
        }
        return addrs;
    }
}

/// 菜单 1: 显示位示图与空闲块数
void showState(const DiskBitmap &b) { b.print(); }

/// 菜单 2: 申请磁盘块(分配)
void doAllocate(DiskBitmap &b) {
    int n = readInt("请输入要申请的磁盘块数: ", 1, b.totalBlocks());
    std::string reason;
    std::vector<DiskBlock> blocks = b.allocate(n, &reason);
    if (blocks.empty()) {
        std::cout << "\n[分配失败] " << reason << "\n";
        return;
    }
    g_lastAllocated = blocks;
    std::cout << "\n[分配成功] 已分配 " << blocks.size() << " 个磁盘块:\n";
    b.printAllocated(blocks);
    std::cout << "分配后的位示图:\n";
    b.print();
}

/// 菜单 3: 回收磁盘块(手动输入物理地址)
void doRecycle(DiskBitmap &b) {
    std::vector<DiskAddress> addrs = readAddresses();
    try {
        b.recycle(addrs);
    } catch (const std::invalid_argument &e) {
        std::cout << "\n[回收失败] " << e.what() << "\n";
        return;
    }
    std::cout << "\n[回收成功] 已回收 " << addrs.size() << " 个磁盘块。\n";
    std::cout << "回收后的位示图:\n";
    b.print();
}

/// 菜单 4: 回收最近一次分配的磁盘块
void recycleLast(DiskBitmap &b) {
    if (g_lastAllocated.empty()) {
        std::cout << "暂无已分配的磁盘块, 请先通过菜单 2 分配。\n";
        return;
    }
    std::vector<DiskAddress> addrs;
    addrs.reserve(g_lastAllocated.size());
    for (const DiskBlock &blk : g_lastAllocated) {
        addrs.push_back(blk.addr);
    }
    b.recycle(addrs);
    std::cout << "\n[回收成功] 已回收最近分配的 " << addrs.size()
              << " 个磁盘块。\n";
    std::cout << "回收后的位示图:\n";
    b.print();
    g_lastAllocated.clear();
}

/// 菜单 5: 运行示例(完整演示: 分配 → 回收)
void runDemo() {
    DiskBitmap b = makeExample();
    std::cout << "\n================ 示例: 初始位示图 ================\n";
    b.print();

    std::cout << "\n>>> 步骤 1: 申请 8 个磁盘块, 返回物理地址并更新位示图\n";
    std::vector<DiskBlock> blocks = b.allocate(8);
    b.printAllocated(blocks);
    b.print();

    std::cout << "\n>>> 步骤 2: 回收这 8 个磁盘块, 更新位示图\n";
    std::vector<DiskAddress> addrs;
    for (const DiskBlock &blk : blocks) {
        addrs.push_back(blk.addr);
    }
    b.recycle(addrs);
    b.print();
}

void printMenu() {
    std::cout << "\n请选择操作:\n"
              << "  1. 显示位示图与空闲块数\n"
              << "  2. 申请磁盘块(分配)\n"
              << "  3. 回收磁盘块(手动输入物理地址)\n"
              << "  4. 回收最近一次分配的磁盘块\n"
              << "  5. 运行示例(分配 8 块 → 回收)\n"
              << "  0. 退出\n";
}

} // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8); // 终端以 UTF-8 显示中文输出
#endif
    DiskBitmap b = makeExample();

    std::cout << "===============================================\n"
              << "     磁盘存储空间分配与回收 —— 位示图模拟\n"
              << "===============================================\n"
              << "已载入数据: " << b.geometry().cylinders << " 柱面 × "
              << b.geometry().heads << " 盘面 × "
              << b.geometry().sectorsPerTrack << " 扇区 = " << b.totalBlocks()
              << " 块, 字长 " << b.geometry().wordBits << " 位; 初始已占用 "
              << b.occupiedCount() << " 块, 空闲 " << b.freeCount()
              << " 块。\n";

    for (;;) {
        printMenu();
        switch (readInt("请输入选择: ", 0, 5)) {
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
            recycleLast(b);
            break;
        case 5:
            runDemo();
            break;
        default: // case 0
            std::cout << "再见!\n";
            return 0;
        }
    }
}
