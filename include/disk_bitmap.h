/**
 * @file disk_bitmap.h
 * @brief 磁盘存储空间分配与回收 —— 位示图数据结构与类接口定义
 *
 * 用位示图记录磁盘空间使用情况: 位示图由若干字构成(每字 16 位), 每一位对应一个
 * 扇区(磁盘块), 1 表示占用, 0 表示空闲; 另用一个变量记录当前空闲块数。
 *
 * 磁盘块地址分两级: 相对块号(逻辑块号)与物理地址(柱面号/盘面号/扇区号)。
 * 以每柱面 4 盘面(磁道)、每盘面 8 扇区、字长 16 位为例, 每柱面扇区数 = 4×8 =
 * 32:
 *
 *   相对块号 = 字号 × 16 + 位号
 *   柱面号   = 相对块号 / 32             (32 = 每柱面扇区数)
 *   盘面号   = (相对块号 % 32) / 8
 *   扇区号   = (相对块号 % 32) % 8
 *
 *   反之: 相对块号 = 柱面号 × 32 + 盘面号 × 8 + 扇区号
 *         字号 = 相对块号 / 16,  位号 = 相对块号 % 16
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace os {

/// 磁盘块物理地址
struct DiskAddress {
    int cylinder = 0; ///< 柱面号
    int head = 0;     ///< 盘面号(磁头号)
    int sector = 0;   ///< 扇区号

    bool operator==(const DiskAddress &o) const {
        return cylinder == o.cylinder && head == o.head && sector == o.sector;
    }
    bool operator!=(const DiskAddress &o) const { return !(*this == o); }
};

/// 磁盘组几何参数
struct DiskGeometry {
    int cylinders = 0;       ///< 柱面数
    int heads = 0;           ///< 每柱面磁道数(盘面数)
    int sectorsPerTrack = 0; ///< 每磁道扇区数
    int wordBits = 16;       ///< 位示图字长(每字的位数)
};

/// 一次分配得到的磁盘块(相对块号 + 物理地址)
struct DiskBlock {
    int logical = 0;  ///< 相对块号(逻辑块号)
    DiskAddress addr; ///< 物理地址
};

/**
 * 磁盘位示图管理器: 实现磁盘块的分配与回收
 *
 * 典型用法:
 *   DiskGeometry geo{40, 4, 8, 16};   // 40 柱面 × 4 盘面 × 8 扇区, 字长 16
 *   DiskBitmap disk(geo, {0, 1, 15}); // 初始已占用相对块号
 *   auto blocks = disk.allocate(5);   // 分配 5 块, 返回相对块号与物理地址
 *   disk.recycle({blocks[0].addr});   // 按物理地址回收
 *   disk.print();                     // 打印位示图与空闲块数
 */
class DiskBitmap {
  public:
    /// 构造: geo 为磁盘几何参数, occupied 为初始已占用的相对块号列表
    /// @throw std::invalid_argument 几何参数非法、块号越界或重复占用时抛出
    DiskBitmap(const DiskGeometry &geo, std::vector<int> occupied = {});

    /// 磁盘几何参数
    const DiskGeometry &geometry() const { return geo_; }

    /// 磁盘块总数
    int totalBlocks() const { return totalBlocks_; }

    /// 当前空闲块数
    int freeCount() const { return freeCount_; }

    /// 当前已占用块数
    int occupiedCount() const { return totalBlocks_ - freeCount_; }

    /// 相对块号 logical 是否已占用
    bool isOccupied(int logical) const;

    /// 分配 n 个磁盘块(自低块号起首次适配), 返回包含相对块号与物理地址的列表。
    /// 若空闲块数不足则返回空列表, 并通过 reason 说明原因(可为 nullptr)。
    std::vector<DiskBlock> allocate(int n, std::string *reason = nullptr);

    /// 回收若干磁盘块(按物理地址): 将对应位清 0 并增加空闲块数
    /// @throw std::invalid_argument 物理地址越界或对应块本就空闲时抛出
    void recycle(const std::vector<DiskAddress> &addrs);

    /// 打印位示图与当前空闲块数
    void print() const;

    /// 打印"用户已获得盘块情况表"(相对块号与物理地址)
    void printAllocated(const std::vector<DiskBlock> &blocks) const;

    /// 相对块号 -> 物理地址(柱面号/盘面号/扇区号)
    DiskAddress physicalOf(int logical) const;

    /// 物理地址 -> 相对块号
    int logicalOf(const DiskAddress &addr) const;

  private:
    /// 相对块号对应的字号 = 块号 / 字长
    int wordOf(int logical) const { return logical / geo_.wordBits; }

    /// 相对块号对应的位号 = 块号 % 字长
    int bitOf(int logical) const { return logical % geo_.wordBits; }

    /// 校验相对块号是否在合法范围内
    void validateLogical(int logical) const;

    /// 校验物理地址是否在合法范围内
    void validateAddress(const DiskAddress &addr) const;

    /// 将相对块号 logical 标记为占用(value=true)或空闲(value=false)
    void setOccupied(int logical, bool value);

    DiskGeometry geo_;       ///< 磁盘几何参数
    int totalBlocks_;        ///< 磁盘块总数
    int sectorsPerCylinder_; ///< 每柱面扇区数 = 盘面数 × 每盘面扇区数
    std::vector<std::uint16_t>
        words_;     ///< 位示图, 每字 16 位, 位 i 对应块 word*16+i
    int freeCount_; ///< 当前空闲块数
};

} // namespace os
