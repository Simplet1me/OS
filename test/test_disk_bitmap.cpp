/**
 * @file test_disk_bitmap.cpp
 * @brief 磁盘存储空间分配与回收(位示图)局部测试(自包含, 不引入第三方测试框架)
 *
 * 覆盖: 几何参数与空闲块数、相对块号↔物理地址换算(含往返)、分配、空闲不足拒绝、
 *       回收、非法输入(越界/重复回收)、构造校验。
 * 运行方式: 直接运行本可执行文件, 或通过 ctest 运行。
 */
#include <iostream>
#include <stdexcept>
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

int g_passed = 0;
int g_failed = 0;

void expect(bool cond, const std::string &what) {
    if (cond) {
        ++g_passed;
        std::cout << "[PASS] " << what << "\n";
    } else {
        ++g_failed;
        std::cout << "[FAIL] " << what << "\n";
    }
}

bool contains(const std::string &text, const std::string &sub) {
    return text.find(sub) != std::string::npos;
}

// 教材例题: 40 柱面 × 4 盘面 × 8 扇区 = 1280 块, 字长 16
const DiskGeometry kGeo = {40, 4, 8, 16};

DiskBitmap makeEmpty() { return DiskBitmap(kGeo); }

// 1. 几何参数与空闲块数
void testGeometry() {
    DiskBitmap b = makeEmpty();
    expect(b.totalBlocks() == 1280, "总块数为 40×4×8 = 1280");
    expect(b.freeCount() == 1280, "空盘空闲块数为 1280");
    expect(b.occupiedCount() == 0, "空盘已占用块数为 0");
}

// 2. 相对块号 -> 物理地址(含边界)
void testPhysicalOf() {
    DiskBitmap b = makeEmpty();
    expect(b.physicalOf(0) == DiskAddress{0, 0, 0}, "块 0 -> (柱0,面0,扇0)");
    expect(b.physicalOf(31) == DiskAddress{0, 3, 7}, "块 31 -> (柱0,面3,扇7)");
    expect(b.physicalOf(32) == DiskAddress{1, 0, 0}, "块 32 -> (柱1,面0,扇0)");
    expect(b.physicalOf(63) == DiskAddress{1, 3, 7}, "块 63 -> (柱1,面3,扇7)");
    expect(b.physicalOf(1279) == DiskAddress{39, 3, 7},
           "块 1279 -> (柱39,面3,扇7)");
}

// 3. 物理地址 -> 相对块号(逆换算)
void testLogicalOf() {
    DiskBitmap b = makeEmpty();
    expect(b.logicalOf({0, 0, 0}) == 0, "(柱0,面0,扇0) -> 块 0");
    expect(b.logicalOf({0, 3, 7}) == 31, "(柱0,面3,扇7) -> 块 31");
    expect(b.logicalOf({39, 3, 7}) == 1279, "(柱39,面3,扇7) -> 块 1279");
}

// 4. 往返一致: 所有块 physicalOf -> logicalOf 应还原
void testRoundTrip() {
    DiskBitmap b = makeEmpty();
    bool ok = true;
    for (int i = 0; i < b.totalBlocks(); ++i) {
        if (b.logicalOf(b.physicalOf(i)) != i) {
            ok = false;
            break;
        }
    }
    expect(ok, "全部 1280 块 physicalOf/logicalOf 往返一致");
}

// 5. 空盘分配: 前 3 块应为 0,1,2
void testAllocateEmpty() {
    DiskBitmap b = makeEmpty();
    std::vector<DiskBlock> blocks = b.allocate(3);
    expect(blocks.size() == 3, "分配 3 块成功");
    bool ok = blocks[0].logical == 0 && blocks[1].logical == 1 &&
              blocks[2].logical == 2;
    expect(ok, "空盘分配得到相对块号 0,1,2");
    expect(blocks[0].addr == DiskAddress{0, 0, 0} &&
               blocks[1].addr == DiskAddress{0, 0, 1},
           "块 0/1 物理地址为 (0,0,0)/(0,0,1)");
    expect(b.freeCount() == 1277, "分配后空闲块数为 1277");
}

// 6. 有初始占用时的分配: 跳过已占用块, 取最低空闲块
void testAllocateWithOccupied() {
    DiskBitmap b(kGeo, {0, 1, 15, 16, 17, 31, 32, 63, 64, 1279});
    expect(b.freeCount() == 1270, "初始占用 10 块, 空闲 1270");
    std::vector<DiskBlock> blocks = b.allocate(3);
    bool ok = blocks[0].logical == 2 && blocks[1].logical == 3 &&
              blocks[2].logical == 4;
    expect(ok, "分配得到相对块号 2,3,4");
    expect(blocks[0].addr == DiskAddress{0, 0, 2},
           "块 2 物理地址为 (柱0,面0,扇2)");
}

// 7. 空闲不足: 申请超量应失败且状态不变
void testAllocateInsufficient() {
    DiskBitmap b(kGeo, {0, 1, 15, 16, 17, 31, 32, 63, 64, 1279});
    std::string reason;
    expect(b.allocate(1271, &reason).empty(), "申请 1271 块应失败(仅剩 1270)");
    expect(contains(reason, "不足"), "失败原因包含\"不足\"");
    expect(b.freeCount() == 1270, "失败后空闲块数不变");
}

// 8. 回收: 分配后按物理地址回收, 空闲块数恢复
void testRecycle() {
    DiskBitmap b = makeEmpty();
    std::vector<DiskBlock> blocks = b.allocate(5); // 0,1,2,3,4
    std::vector<DiskAddress> addrs;
    for (const DiskBlock &blk : blocks)
        addrs.push_back(blk.addr);
    b.recycle(addrs);
    expect(b.freeCount() == 1280, "回收后空闲块数恢复为 1280");
    expect(!b.isOccupied(0) && !b.isOccupied(4), "块 0、4 已回收为空闲");
}

// 9. 回收越界物理地址应抛出异常
void testRecycleOutOfRange() {
    DiskBitmap b = makeEmpty();
    bool threw = false;
    try {
        b.recycle({{40, 0, 0}}); // 柱面号越界
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    expect(threw, "回收越界柱面号应抛出异常");
}

// 10. 重复回收空闲块应抛出异常
void testRecycleFreeBlock() {
    DiskBitmap b = makeEmpty();
    bool threw = false;
    try {
        b.recycle({{0, 0, 0}}); // 块 0 本就空闲
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    expect(threw, "回收空闲块应抛出异常");
}

// 11. 构造校验: 几何参数非法 / 占用块越界 / 重复占用
void testInvalidConstruction() {
    bool threw = false;
    try {
        DiskBitmap bad({0, 4, 8, 16});
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    expect(threw, "柱面数 0 应抛出异常");

    threw = false;
    try {
        DiskBitmap bad(kGeo, {1280}); // 越界
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    expect(threw, "占用块越界应抛出异常");

    threw = false;
    try {
        DiskBitmap bad(kGeo, {0, 0}); // 重复占用
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    expect(threw, "重复占用应抛出异常");
}

} // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::cout << "===== 磁盘存储空间分配与回收(位示图)局部测试 =====\n";
    testGeometry();
    testPhysicalOf();
    testLogicalOf();
    testRoundTrip();
    testAllocateEmpty();
    testAllocateWithOccupied();
    testAllocateInsufficient();
    testRecycle();
    testRecycleOutOfRange();
    testRecycleFreeBlock();
    testInvalidConstruction();

    std::cout << "\n共 " << (g_passed + g_failed) << " 项断言, 通过 "
              << g_passed << " 项, 失败 " << g_failed << " 项\n";
    return g_failed == 0 ? 0 : 1;
}
