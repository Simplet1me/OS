/**
 * @file disk_bitmap.cpp
 * @brief 磁盘存储空间分配与回收 —— 位示图实现
 *
 * 位示图中每一位对应一个扇区(磁盘块), 0=空闲, 1=占用:
 *   - 分配: 查位示图找出为 0 的位(自低块号起首次适配), 置 1,
 * 空闲块数减去占用块数; 先由字号/位号算相对块号(相对块号 = 字号×16+位号),
 * 再换算物理地址 (柱面号 = 相对块号/32, 盘面号 = (相对块号%32)/8, 扇区号 =
 * (相对块号%32)%8)。
 *   - 回收: 由物理地址算相对块号(相对块号 = 柱面号×32 + 盘面号×8 + 扇区号),
 *     再算字号/位号(字号 = 相对块号/16, 位号 = 相对块号%16), 将对应位清 0。
 */
#include "disk_bitmap.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "table.h"

namespace os {

namespace {

/// 抛出非法参数异常
[[noreturn]] void throwInvalid(const std::string &what) {
    throw std::invalid_argument(what);
}

} // namespace

DiskBitmap::DiskBitmap(const DiskGeometry &geo, std::vector<int> occupied) {
    if (geo.cylinders <= 0) {
        throwInvalid("柱面数必须大于 0");
    }
    if (geo.heads <= 0) {
        throwInvalid("每柱面盘面数(磁道数)必须大于 0");
    }
    if (geo.sectorsPerTrack <= 0) {
        throwInvalid("每磁道扇区数必须大于 0");
    }
    if (geo.wordBits <= 0 || geo.wordBits > 16) {
        throwInvalid("位示图字长须在 1~16 之间");
    }
    geo_ = geo;
    sectorsPerCylinder_ = geo_.heads * geo_.sectorsPerTrack;
    totalBlocks_ = geo_.cylinders * sectorsPerCylinder_;
    words_.assign((totalBlocks_ + geo_.wordBits - 1) / geo_.wordBits, 0);
    freeCount_ = totalBlocks_;

    for (int b : occupied) {
        validateLogical(b);
        if (isOccupied(b)) {
            throwInvalid("相对块号 " + std::to_string(b) +
                         " 被重复标记为已占用");
        }
        setOccupied(b, true);
        --freeCount_;
    }
}

void DiskBitmap::validateLogical(int logical) const {
    if (logical < 0 || logical >= totalBlocks_) {
        throwInvalid("相对块号 " + std::to_string(logical) + " 超出范围 [0, " +
                     std::to_string(totalBlocks_ - 1) + "]");
    }
}

void DiskBitmap::validateAddress(const DiskAddress &addr) const {
    if (addr.cylinder < 0 || addr.cylinder >= geo_.cylinders) {
        throwInvalid("柱面号 " + std::to_string(addr.cylinder) +
                     " 超出范围 [0, " + std::to_string(geo_.cylinders - 1) +
                     "]");
    }
    if (addr.head < 0 || addr.head >= geo_.heads) {
        throwInvalid("盘面号 " + std::to_string(addr.head) + " 超出范围 [0, " +
                     std::to_string(geo_.heads - 1) + "]");
    }
    if (addr.sector < 0 || addr.sector >= geo_.sectorsPerTrack) {
        throwInvalid("扇区号 " + std::to_string(addr.sector) +
                     " 超出范围 [0, " +
                     std::to_string(geo_.sectorsPerTrack - 1) + "]");
    }
}

void DiskBitmap::setOccupied(int logical, bool value) {
    const int word = wordOf(logical);
    const int bit = bitOf(logical);
    const std::uint16_t mask =
        static_cast<std::uint16_t>(std::uint16_t(1) << bit);
    if (value) {
        words_[word] = static_cast<std::uint16_t>(words_[word] | mask);
    } else {
        words_[word] = static_cast<std::uint16_t>(
            words_[word] & static_cast<std::uint16_t>(~mask));
    }
}

bool DiskBitmap::isOccupied(int logical) const {
    const int word = wordOf(logical);
    const int bit = bitOf(logical);
    return (words_[word] &
            static_cast<std::uint16_t>(std::uint16_t(1) << bit)) != 0;
}

DiskAddress DiskBitmap::physicalOf(int logical) const {
    validateLogical(logical);
    DiskAddress a;
    a.cylinder = logical / sectorsPerCylinder_;
    a.head = (logical % sectorsPerCylinder_) / geo_.sectorsPerTrack;
    a.sector = (logical % sectorsPerCylinder_) % geo_.sectorsPerTrack;
    return a;
}

int DiskBitmap::logicalOf(const DiskAddress &addr) const {
    validateAddress(addr);
    return addr.cylinder * sectorsPerCylinder_ +
           addr.head * geo_.sectorsPerTrack + addr.sector;
}

std::vector<DiskBlock> DiskBitmap::allocate(int n, std::string *reason) {
    auto fail = [reason](const std::string &msg) {
        if (reason) {
            *reason = msg;
        }
        return std::vector<DiskBlock>{};
    };

    if (n <= 0) {
        return fail("申请磁盘块数必须大于 0");
    }
    if (n > freeCount_) {
        return fail("空闲磁盘块不足: 需要 " + std::to_string(n) +
                    " 块, 当前仅剩 " + std::to_string(freeCount_) + " 块");
    }

    // 首次适配: 自低块号起寻找空闲位并置 1
    std::vector<DiskBlock> blocks;
    blocks.reserve(static_cast<std::size_t>(n));
    for (int logical = 0;
         logical < totalBlocks_ && static_cast<int>(blocks.size()) < n;
         ++logical) {
        if (!isOccupied(logical)) {
            setOccupied(logical, true);
            DiskBlock b;
            b.logical = logical;
            b.addr = physicalOf(logical);
            blocks.push_back(b);
        }
    }
    freeCount_ -= static_cast<int>(blocks.size());
    return blocks;
}

void DiskBitmap::recycle(const std::vector<DiskAddress> &addrs) {
    // 先整体校验, 保证合法; 避免回收过程中途因异常造成状态不一致
    for (const DiskAddress &a : addrs) {
        validateAddress(a);
        const int logical = logicalOf(a);
        if (!isOccupied(logical)) {
            throwInvalid("磁盘块(" + std::to_string(a.cylinder) + "," +
                         std::to_string(a.head) + "," +
                         std::to_string(a.sector) + ") 当前为空闲, 无需回收");
        }
    }
    for (const DiskAddress &a : addrs) {
        setOccupied(logicalOf(a), false);
        ++freeCount_;
    }
}

void DiskBitmap::print() const {
    std::vector<std::string> headers = {"字号", "块号范围"};
    for (int i = 0; i < geo_.wordBits; ++i) {
        headers.push_back("位" + std::to_string(i));
    }

    Table table(headers);
    table.setTitle(
        "位示图 (1=占用, 0=空闲)  磁盘组: " + std::to_string(geo_.cylinders) +
        " 柱面 × " + std::to_string(geo_.heads) + " 盘面 × " +
        std::to_string(geo_.sectorsPerTrack) +
        " 扇区 = " + std::to_string(totalBlocks_) + " 块, 字长 " +
        std::to_string(geo_.wordBits) + " 位");

    const int nwords = static_cast<int>(words_.size());
    for (int w = 0; w < nwords; ++w) {
        std::vector<std::string> row;
        row.push_back(std::to_string(w));

        const int start = w * geo_.wordBits;
        const int end = std::min(start + geo_.wordBits - 1, totalBlocks_ - 1);
        row.push_back(std::to_string(start) + "~" + std::to_string(end));

        for (int i = 0; i < geo_.wordBits; ++i) {
            const int logical = w * geo_.wordBits + i;
            if (logical >= totalBlocks_) {
                row.push_back("-"); // 总块数非字长整数倍时, 末字的空位
            } else {
                row.push_back(isOccupied(logical) ? "1" : "0");
            }
        }
        table.addRow(row);
    }
    table.print();

    std::cout << "当前空闲磁盘块数: " << freeCount_
              << ", 已占用磁盘块数: " << (totalBlocks_ - freeCount_) << "\n";
}

void DiskBitmap::printAllocated(const std::vector<DiskBlock> &blocks) const {
    Table table({"序号", "相对块号", "柱面号", "盘面号", "扇区号"});
    table.setTitle("用户已获得盘块情况表");
    for (std::size_t c = 1; c <= 4; ++c) {
        table.setAlign(c, Table::Align::Right);
    }
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        const DiskBlock &b = blocks[i];
        table.addRow({std::to_string(i + 1), std::to_string(b.logical),
                      std::to_string(b.addr.cylinder),
                      std::to_string(b.addr.head),
                      std::to_string(b.addr.sector)});
    }
    table.print();
}

} // namespace os
