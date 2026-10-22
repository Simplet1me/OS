/**
 * @file test_bitmap.cpp
 * @brief 分页存储管理(位示图)局部测试(自包含, 不引入第三方测试框架)
 *
 * 覆盖: 初始状态与空闲块数、块号/位示图位置换算、分配、空闲不足拒绝、
 *       回收、非法输入(越界/重复回收)、构造校验、分配-回收-再分配。
 * 运行方式: 直接运行本可执行文件, 或通过 ctest 运行。
 */
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "bitmap.h"

#ifdef _WIN32
#include <windows.h>
#endif

using os::BitMap;

namespace
{

    int g_passed = 0;
    int g_failed = 0;

    void expect(bool cond, const std::string &what)
    {
        if (cond)
        {
            ++g_passed;
            std::cout << "[PASS] " << what << "\n";
        }
        else
        {
            ++g_failed;
            std::cout << "[FAIL] " << what << "\n";
        }
    }

    bool contains(const std::string &text, const std::string &sub)
    {
        return text.find(sub) != std::string::npos;
    }

    // ---------------- 经典例题数据(教材示例) ----------------
    // 主存 64 块, 初始占用第 0,1,4,5,6,9,11,13,24,31 共 10 块, 空闲 54 块
    const int kTotalBlocks = 64;
    const std::vector<int> kOccupied = {0, 1, 4, 5, 6, 9, 11, 13, 24, 31};

    BitMap makeExample() { return BitMap(kTotalBlocks, kOccupied); }

    // 1. 初始状态
    void testInitialState()
    {
        BitMap b = makeExample();
        expect(b.totalBlocks() == 64, "总块数为 64");
        expect(b.freeCount() == 54, "初始空闲块数为 54");
        expect(b.occupiedCount() == 10, "初始已占用块数为 10");
        bool ok = true;
        for (int x : kOccupied)
            if (!b.isOccupied(x))
                ok = false;
        expect(ok, "题目给定的 10 块均已占用");
        expect(!b.isOccupied(2) && !b.isOccupied(63), "块 2、63 空闲");
    }

    // 2. 块号与位示图位置换算
    void testLocate()
    {
        auto p0 = BitMap::locate(0);
        auto p31 = BitMap::locate(31);
        auto p63 = BitMap::locate(63);
        expect(p0.first == 0 && p0.second == 0, "块 0 -> (字节 0, 位 0)");
        expect(p31.first == 3 && p31.second == 7, "块 31 -> (字节 3, 位 7)");
        expect(p63.first == 7 && p63.second == 7, "块 63 -> (字节 7, 位 7)");
        expect(BitMap::blockNo(0, 0) == 0, "(字节 0, 位 0) -> 块 0");
        expect(BitMap::blockNo(3, 7) == 31, "(字节 3, 位 7) -> 块 31");
        expect(BitMap::blockNo(7, 7) == 63, "(字节 7, 位 7) -> 块 63");
    }

    // 3. 分配 5 块(教材示例): 首次适配应得到 2,3,7,8,10
    void testAllocate()
    {
        BitMap b = makeExample();
        std::vector<int> blocks = b.allocate(5);
        expect(blocks == std::vector<int>({2, 3, 7, 8, 10}), "分配 5 块应得到 2,3,7,8,10");
        expect(b.freeCount() == 49, "分配后空闲块数为 49");
        bool ok = true;
        for (int x : blocks)
            if (!b.isOccupied(x))
                ok = false;
        expect(ok, "分配得到的块均被置为占用");
    }

    // 4. 空闲块不足: 申请 55 块(仅剩 54)应失败且状态不变
    void testAllocateInsufficient()
    {
        BitMap b = makeExample();
        std::string reason;
        expect(b.allocate(55, &reason).empty(), "申请 55 块应失败");
        expect(contains(reason, "不足"), "失败原因包含\"不足\"");
        expect(b.freeCount() == 54, "失败后空闲块数不变");
    }

    // 5. 申请块数非法: 0 或负数应失败
    void testAllocateIllegalCount()
    {
        BitMap b = makeExample();
        expect(b.allocate(0).empty(), "申请 0 块应失败");
        expect(b.allocate(-1).empty(), "申请负数块应失败");
    }

    // 6. 回收 4 块(教材示例): 4,5,6,31 由占用变空闲
    void testRecycle()
    {
        BitMap b = makeExample();
        b.recycle({4, 5, 6, 31});
        expect(b.freeCount() == 58, "回收后空闲块数为 58");
        expect(!b.isOccupied(4) && !b.isOccupied(5) && !b.isOccupied(6) && !b.isOccupied(31),
               "块 4、5、6、31 已回收为空闲");
        expect(b.isOccupied(0) && b.isOccupied(1), "块 0、1 仍占用");
    }

    // 7. 回收越界块应抛出异常
    void testRecycleOutOfRange()
    {
        BitMap b = makeExample();
        bool threw = false;
        try
        {
            b.recycle({64});
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }
        expect(threw, "回收块 64(越界)应抛出异常");
    }

    // 8. 重复回收空闲块应抛出异常
    void testRecycleFreeBlock()
    {
        BitMap b = makeExample();
        bool threw = false;
        try
        {
            b.recycle({2}); // 块 2 本就空闲
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }
        expect(threw, "回收空闲块应抛出异常");
    }

    // 9. 构造校验: 块数非法 / 占用块越界 / 重复占用
    void testInvalidConstruction()
    {
        bool threw = false;
        try
        {
            BitMap bad(0);
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }
        expect(threw, "块数 0 应抛出异常");

        threw = false;
        try
        {
            BitMap bad(64, {64}); // 越界
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }
        expect(threw, "占用块越界应抛出异常");

        threw = false;
        try
        {
            BitMap bad(64, {0, 0}); // 重复占用
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }
        expect(threw, "重复占用应抛出异常");
    }

    // 10. 分配-回收-再分配: 回收后重新分配可用
    void testAllocateRecycleAllocate()
    {
        BitMap b = makeExample();
        std::vector<int> first = b.allocate(5); // 2,3,7,8,10
        b.recycle({4, 5, 6, 31});               // 释放 4,5,6,31
        std::vector<int> second = b.allocate(4);
        expect(first == std::vector<int>({2, 3, 7, 8, 10}), "首次分配得到 2,3,7,8,10");
        expect(second == std::vector<int>({4, 5, 6, 12}), "回收后重新分配得到 4,5,6,12");
        expect(b.freeCount() == 49, "再分配后空闲块数为 49");
    }

} // namespace

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::cout << "===== 分页存储管理(位示图)局部测试 =====\n";
    testInitialState();
    testLocate();
    testAllocate();
    testAllocateInsufficient();
    testAllocateIllegalCount();
    testRecycle();
    testRecycleOutOfRange();
    testRecycleFreeBlock();
    testInvalidConstruction();
    testAllocateRecycleAllocate();

    std::cout << "\n共 " << (g_passed + g_failed) << " 项断言, 通过 " << g_passed
              << " 项, 失败 " << g_failed << " 项\n";
    return g_failed == 0 ? 0 : 1;
}
