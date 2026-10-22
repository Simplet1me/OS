/**
 * @file bitmap.h
 * @brief 分页存储管理 —— 位示图数据结构与类接口定义
 *
 * 分页式存储管理把主存划分成大小相等的若干块, 用一张"位示图"记录各块的空闲/
 * 占用情况: 每一位与一个物理块对应, 0 表示空闲, 1 表示已占用。
 *
 * 位示图可由若干字节构成(每字节 8 位), 位 i(自最低位起第 i 位, i = 0~7)
 * 对应块号 j*8 + i, 其中 j 为字节号。因此块号与位示图位置的换算公式为:
 *
 *   块号   = j * 8 + i
 *   字节号 j = 块号 / 8  (整除)
 *   位数   i = 块号 % 8  (取余)
 *
 * 另用一个单元记录"当前空闲块数": 分配时减去占用块数, 回收时加上归还块数。
 */
#pragma once

#include <string>
#include <utility>
#include <vector>

namespace os
{

    /**
     * 位示图(主存分配表)
     *
     * 典型用法:
     *   BitMap m(64, {0, 1, 4, 5, 6, 9, 11, 13, 24, 31}); // 64 块, 指定初始占用
     *   auto blocks = m.allocate(5);                       // 为 5 页作业分配 5 块
     *   m.recycle({4, 5, 6, 31});                          // 回收 4 个块
     *   m.print();                                         // 打印位示图与空闲块数
     */
    class BitMap
    {
    public:
        /// 构造: totalBlocks 块, 初始全部空闲
        explicit BitMap(int totalBlocks);

        /// 构造: totalBlocks 块, 并标记 occupied 中的块为已占用
        /// @throw std::invalid_argument 块数非法、块号越界或重复占用时抛出
        BitMap(int totalBlocks, std::vector<int> occupied);

        /// 主存总块数
        int totalBlocks() const { return totalBlocks_; }

        /// 当前空闲块数
        int freeCount() const { return freeCount_; }

        /// 当前已占用块数
        int occupiedCount() const { return totalBlocks_ - freeCount_; }

        /// 块 block 是否已占用
        bool isOccupied(int block) const;

        /// 为作业分配 n 个空闲块(自低块号起首次适配), 返回升序的块号列表。
        /// 若空闲块数不足则返回空列表, 并通过 reason 说明原因(可为 nullptr)。
        /// 分配成功后相应位被置 1, 空闲块数相应减少。
        std::vector<int> allocate(int n, std::string *reason = nullptr);

        /// 回收若干主存块: 将对应位清 0, 并增加空闲块数
        /// @throw std::invalid_argument 块号越界或对应块本就空闲时抛出
        void recycle(const std::vector<int> &blocks);

        /// 打印位示图(以表格形式)与当前空闲块数
        void print() const;

        /// 块号 -> (字节号 j, 位数 i)
        static std::pair<int, int> locate(int block);

        /// (字节号 j, 位数 i) -> 块号
        static int blockNo(int j, int i);

    private:
        /// 校验块号是否在 [0, totalBlocks_) 范围内, 越界抛异常
        void validate(int block) const;

        /// 将块 block 标记为占用(value=true)或空闲(value=false)
        void setOccupied(int block, bool value);

        int totalBlocks_;                 ///< 主存总块数
        std::vector<unsigned char> bytes_; ///< 位示图, 每字节 8 位, 位 i 对应块 j*8+i
        int freeCount_;                   ///< 当前空闲块数
    };

    /// 打印作业页表(页号 -> 块号, blocks[k] 为第 k 页对应的物理块号)
    void printPageTable(const std::vector<int> &blocks);

} // namespace os
