/**
 * @file bitmap.cpp
 * @brief 分页存储管理 —— 位示图实现
 *
 * 位示图中每一位对应一个物理块(0=空闲, 1=已占用):
 *   - 分配: 先检查当前空闲块数能否满足作业要求, 不能满足则分配失败;
 *     能满足则自低块号起寻找空闲位并置 1, 空闲块数减去本次占用块数,
 *     由找到的位按"块号 = j*8 + i"计算块号, 得到作业页表的块号列。
 *   - 回收: 根据归还的块号按"字节号 j = 块号/8, 位数 i = 块号%8"换算位示图
 *     位置, 将对应位清 0, 空闲块数加上归还块数。
 */
#include "bitmap.h"

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

BitMap::BitMap(int totalBlocks) : BitMap(totalBlocks, {}) {}

BitMap::BitMap(int totalBlocks, std::vector<int> occupied) {
    if (totalBlocks <= 0) {
        throwInvalid("主存块数必须大于 0");
    }
    totalBlocks_ = totalBlocks;
    bytes_.assign((totalBlocks_ + 7) / 8, 0);
    freeCount_ = totalBlocks_;

    for (int b : occupied) {
        validate(b);
        if (isOccupied(b)) {
            throwInvalid("块 " + std::to_string(b) + " 被重复标记为已占用");
        }
        setOccupied(b, true);
        --freeCount_;
    }
}

void BitMap::validate(int block) const {
    if (block < 0 || block >= totalBlocks_) {
        throwInvalid("块号 " + std::to_string(block) + " 超出范围 [0, " +
                     std::to_string(totalBlocks_ - 1) + "]");
    }
}

void BitMap::setOccupied(int block, bool value) {
    const int j = block / 8; // 字节号
    const int i = block % 8; // 位数
    const unsigned char mask = static_cast<unsigned char>(1u << i);
    if (value) {
        bytes_[j] = static_cast<unsigned char>(bytes_[j] | mask);
    } else {
        bytes_[j] = static_cast<unsigned char>(
            bytes_[j] & static_cast<unsigned char>(~mask));
    }
}

bool BitMap::isOccupied(int block) const {
    const int j = block / 8;
    const int i = block % 8;
    return (bytes_[j] & static_cast<unsigned char>(1u << i)) != 0;
}

std::vector<int> BitMap::allocate(int n, std::string *reason) {
    auto fail = [reason](const std::string &msg) {
        if (reason) {
            *reason = msg;
        }
        return std::vector<int>{};
    };

    if (n <= 0) {
        return fail("申请块数必须大于 0");
    }
    if (n > freeCount_) {
        return fail("空闲块数不足: 需要 " + std::to_string(n) +
                    " 块, 当前仅剩 " + std::to_string(freeCount_) + " 块");
    }

    // 首次适配: 自低块号起寻找空闲位并置 1
    std::vector<int> blocks;
    blocks.reserve(static_cast<std::size_t>(n));
    for (int block = 0;
         block < totalBlocks_ && static_cast<int>(blocks.size()) < n; ++block) {
        if (!isOccupied(block)) {
            setOccupied(block, true);
            blocks.push_back(block);
        }
    }
    freeCount_ -= static_cast<int>(blocks.size());
    return blocks;
}

void BitMap::recycle(const std::vector<int> &blocks) {
    // 先整体校验, 保证合法; 避免回收过程中途因异常造成状态不一致
    for (int b : blocks) {
        validate(b);
        if (!isOccupied(b)) {
            throwInvalid("块 " + std::to_string(b) + " 当前为空闲, 无需回收");
        }
    }
    for (int b : blocks) {
        setOccupied(b, false);
        ++freeCount_;
    }
}

std::pair<int, int> BitMap::locate(int block) { return {block / 8, block % 8}; }

int BitMap::blockNo(int j, int i) { return j * 8 + i; }

void BitMap::print() const {
    Table table({"字节", "块号范围", "位0", "位1", "位2", "位3", "位4", "位5",
                 "位6", "位7"});
    table.setTitle("位示图 (1=已占用, 0=空闲)");

    const int nbytes = static_cast<int>(bytes_.size());
    for (int j = 0; j < nbytes; ++j) {
        std::vector<std::string> row;
        row.push_back(std::to_string(j));

        const int start = j * 8;
        const int end = std::min(start + 7, totalBlocks_ - 1);
        row.push_back(std::to_string(start) + "~" + std::to_string(end));

        for (int i = 0; i < 8; ++i) {
            const int block = j * 8 + i;
            if (block >= totalBlocks_) {
                row.push_back("-"); // 总块数非 8 的倍数时, 末尾字节的空位
            } else {
                row.push_back(isOccupied(block) ? "1" : "0");
            }
        }
        table.addRow(row);
    }
    table.print();

    std::cout << "当前空闲块数: " << freeCount_
              << ", 已占用块数: " << (totalBlocks_ - freeCount_) << "\n";
}

void printPageTable(const std::vector<int> &blocks) {
    Table table({"页号", "块号"});
    table.setTitle("作业页表");
    table.setAlign(0, Table::Align::Right);
    table.setAlign(1, Table::Align::Right);
    for (std::size_t k = 0; k < blocks.size(); ++k) {
        table.addRow({std::to_string(k), std::to_string(blocks[k])});
    }
    table.print();
}

} // namespace os
