/**
 * @file page_replacement.h
 * @brief 页面置换算法(FIFO / LRU)模拟 —— 数据结构与类接口定义
 *
 * 页面置换是虚拟存储管理的关键: 进程页数(PP)大于分配的内存页框数(AP)时,
 * 访问的页面不一定在内存中, 若缺页则须调入, 内存已满时还要按某种策略淘汰一个
 * 页面。本实验在用户空间用线性表模拟进程页面的调度, 实现两种经典置换算法:
 *
 *   - FIFO(先进先出): 淘汰内存中最早进入的页面, 内存页框构成一个先进先出队列。
 *   - LRU(最近最少使用): 淘汰内存中最长时间未被访问的页面。
 *
 * 通过统计缺页次数等指标, 比较两种算法的缺页率与优劣。
 */
#pragma once

#include <string>
#include <vector>

namespace os {

/// 页面置换算法
enum class Algorithm {
    FIFO, ///< 先进先出
    LRU   ///< 最近最少使用
};

/// 一次页面访问的模拟记录
struct PageStep {
    int page;                ///< 本次访问的页面号
    bool fault;              ///< 是否发生缺页
    int replacedPage;        ///< 被淘汰的页面号(-1 表示未淘汰, 即有空闲页框)
    std::vector<int> frames; ///< 本次访问后各页框中的页面(-1 表示空闲)
};

/// 一次模拟运行的结果(含性能指标与逐步过程)
struct PageResult {
    int total = 0;               ///< 页面访问总次数
    int faults = 0;              ///< 缺页次数
    int replacements = 0;        ///< 置换次数(淘汰已驻留页面调入新页)
    std::vector<PageStep> steps; ///< 逐步过程记录

    /// 命中次数 = 总访问次数 - 缺页次数
    int hits() const { return total - faults; }

    /// 缺页率 = 缺页次数 / 总访问次数
    double faultRate() const {
        return total == 0 ? 0.0 : static_cast<double>(faults) / total;
    }

    /// 命中率 = 命中次数 / 总访问次数
    double hitRate() const {
        return total == 0 ? 0.0 : static_cast<double>(hits()) / total;
    }
};

/**
 * 页面置换算法模拟器
 *
 * 典型用法:
 *   PageReplacement pr(3, {7,0,1,2,0,3,0,4,2,3,0,3,2,1,2,0,1,7,0,1});
 *   PageResult fifo = pr.runFIFO();   // FIFO 模拟
 *   PageResult lru  = pr.runLRU();    // LRU 模拟(同一份访问序列, 独立运行)
 */
class PageReplacement {
  public:
    /// 构造: frameCount 为分配的内存页框数(AP), refs 为页面访问序列
    /// @throw std::invalid_argument 页框数非法或访问序列为空/含负页号时抛出
    PageReplacement(int frameCount, std::vector<int> refs);

    /// 运行指定置换算法, 返回含缺页率等指标的模拟结果
    PageResult run(Algorithm algo) const;

    /// 运行 FIFO 算法
    PageResult runFIFO() const { return simulateFIFO(); }

    /// 运行 LRU 算法
    PageResult runLRU() const { return simulateLRU(); }

    /// 分配的内存页框数(AP)
    int frameCount() const { return frameCount_; }

    /// 页面访问序列
    const std::vector<int> &refs() const { return refs_; }

  private:
    /// FIFO 算法模拟
    PageResult simulateFIFO() const;

    /// LRU 算法模拟
    PageResult simulateLRU() const;

    /// 在 frames 中查找 page 所在页框下标, 未找到返回 -1
    static int findFrame(const std::vector<int> &frames, int page);

    /// 返回第一个空闲页框下标, 无空闲返回 -1
    static int firstEmpty(const std::vector<int> &frames);

    int frameCount_;        ///< 分配的内存页框数
    std::vector<int> refs_; ///< 页面访问序列
};

/// 生成页面访问序列: total 次访问, 页号取值 [0, pageCount)
/// @throw std::invalid_argument total 或 pageCount 非正时抛出
std::vector<int> randomRefs(int total, int pageCount, unsigned seed);

/// 将 0~1 的比率格式化为百分比字符串(保留两位小数), 如 0.2344 -> "23.44%"
std::string formatPercent(double rate);

/// 打印一次模拟运行的逐步过程表
void printTrace(const PageResult &result, int frameCount,
                const std::string &title);

/// 打印 FIFO 与 LRU 的性能对比表
void printComparison(const PageResult &fifo, const PageResult &lru);

} // namespace os
