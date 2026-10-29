/**
 * @file page_replacement.cpp
 * @brief 页面置换算法(FIFO / LRU)模拟实现
 *
 * 两种算法都维护一个长度为 AP 的页框数组(空闲记为 -1), 逐个处理访问序列:
 *   - 若访问页面已在内存: 命中, FIFO 不变; LRU 更新其最近使用时间。
 *   - 若缺页: 有空闲页框则直接装入; 已满则淘汰一个页面再装入, 置换次数加 1。
 *       FIFO 淘汰先进先出队列队首; LRU 淘汰最近使用时间最早的页面。
 */
#include "page_replacement.h"

#include <deque>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

#include "table.h"

namespace os {

namespace {

/// 抛出非法参数异常
[[noreturn]] void throwInvalid(const std::string &what) {
    throw std::invalid_argument(what);
}

} // namespace

PageReplacement::PageReplacement(int frameCount, std::vector<int> refs) {
    if (frameCount <= 0) {
        throwInvalid("分配的内存页框数必须大于 0");
    }
    if (refs.empty()) {
        throwInvalid("页面访问序列不能为空");
    }
    for (int p : refs) {
        if (p < 0) {
            throwInvalid("页面访问序列中的页号不能为负数");
        }
    }
    frameCount_ = frameCount;
    refs_ = std::move(refs);
}

int PageReplacement::findFrame(const std::vector<int> &frames, int page) {
    for (std::size_t k = 0; k < frames.size(); ++k) {
        if (frames[k] == page) {
            return static_cast<int>(k);
        }
    }
    return -1;
}

int PageReplacement::firstEmpty(const std::vector<int> &frames) {
    for (std::size_t k = 0; k < frames.size(); ++k) {
        if (frames[k] < 0) {
            return static_cast<int>(k);
        }
    }
    return -1;
}

PageResult PageReplacement::run(Algorithm algo) const {
    return algo == Algorithm::FIFO ? simulateFIFO() : simulateLRU();
}

PageResult PageReplacement::simulateFIFO() const {
    PageResult r;
    r.total = static_cast<int>(refs_.size());
    std::vector<int> frames(frameCount_, -1);
    std::deque<int> queue; // 页框槽位号, 按进入先后构成先进先出队列

    for (int page : refs_) {
        int slot = findFrame(frames, page);
        if (slot >= 0) // 命中
        {
            r.steps.push_back({page, false, -1, frames});
            continue;
        }

        ++r.faults; // 缺页
        int replaced = -1;
        slot = firstEmpty(frames);
        if (slot >= 0) {
            frames[slot] = page; // 有空闲页框, 直接装入
        } else {
            slot = queue.front(); // 淘汰最早进入的页框
            queue.pop_front();
            replaced = frames[slot];
            frames[slot] = page;
            ++r.replacements;
        }
        queue.push_back(slot);
        r.steps.push_back({page, true, replaced, frames});
    }
    return r;
}

PageResult PageReplacement::simulateLRU() const {
    PageResult r;
    r.total = static_cast<int>(refs_.size());
    std::vector<int> frames(frameCount_, -1);
    std::vector<int> lastUsed(frameCount_, -1); // 各页框最近使用时刻
    int time = 0;

    for (int page : refs_) {
        ++time;
        int slot = findFrame(frames, page);
        if (slot >= 0) // 命中, 更新最近使用时间
        {
            lastUsed[slot] = time;
            r.steps.push_back({page, false, -1, frames});
            continue;
        }

        ++r.faults; // 缺页
        int replaced = -1;
        slot = firstEmpty(frames);
        if (slot >= 0) {
            frames[slot] = page;
            lastUsed[slot] = time;
        } else {
            slot = 0; // 淘汰最近使用时间最早的页面
            for (int k = 1; k < frameCount_; ++k) {
                if (lastUsed[k] < lastUsed[slot]) {
                    slot = k;
                }
            }
            replaced = frames[slot];
            frames[slot] = page;
            lastUsed[slot] = time;
            ++r.replacements;
        }
        r.steps.push_back({page, true, replaced, frames});
    }
    return r;
}

std::vector<int> randomRefs(int total, int pageCount, unsigned seed) {
    if (total <= 0 || pageCount <= 0) {
        throwInvalid("访问次数与页面数必须大于 0");
    }
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> dist(0, pageCount - 1);
    std::vector<int> refs(static_cast<std::size_t>(total));
    for (int &p : refs) {
        p = dist(gen);
    }
    return refs;
}

std::string formatPercent(double rate) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << rate * 100.0 << "%";
    return oss.str();
}

void printTrace(const PageResult &result, int frameCount,
                const std::string &title) {
    std::vector<std::string> headers = {"序号", "访问页面", "缺页", "淘汰"};
    for (int k = 0; k < frameCount; ++k) {
        headers.push_back("页框" + std::to_string(k));
    }

    Table table(headers);
    table.setTitle(title);
    for (std::size_t i = 0; i < result.steps.size(); ++i) {
        const PageStep &s = result.steps[i];
        std::vector<std::string> row;
        row.push_back(std::to_string(i + 1));
        row.push_back(std::to_string(s.page));
        row.push_back(s.fault ? "是" : "否");
        row.push_back(s.replacedPage < 0 ? "-"
                                         : std::to_string(s.replacedPage));
        for (int k = 0; k < frameCount; ++k) {
            row.push_back(s.frames[k] < 0 ? "-" : std::to_string(s.frames[k]));
        }
        table.addRow(row);
    }
    table.print();
}

void printComparison(const PageResult &fifo, const PageResult &lru) {
    Table table({"指标", "FIFO", "LRU"});
    table.setTitle("FIFO 与 LRU 性能对比");
    table.addRow(
        {"访问总次数", std::to_string(fifo.total), std::to_string(lru.total)});
    table.addRow(
        {"缺页次数", std::to_string(fifo.faults), std::to_string(lru.faults)});
    table.addRow({"缺页率", formatPercent(fifo.faultRate()),
                  formatPercent(lru.faultRate())});
    table.addRow(
        {"命中次数", std::to_string(fifo.hits()), std::to_string(lru.hits())});
    table.addRow({"命中率", formatPercent(fifo.hitRate()),
                  formatPercent(lru.hitRate())});
    table.addRow({"置换次数", std::to_string(fifo.replacements),
                  std::to_string(lru.replacements)});
    table.print();
}

} // namespace os
