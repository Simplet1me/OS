/**
 * @file test_page_replacement.cpp
 * @brief 页面置换算法(FIFO / LRU)局部测试(自包含, 不引入第三方测试框架)
 *
 * 覆盖: 经典例题缺页次数、全缺页、无置换、命中检测、FIFO/LRU 一致性入口、
 *       构造校验、随机序列生成、逐步过程记录合法性。
 * 运行方式: 直接运行本可执行文件, 或通过 ctest 运行。
 */
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "page_replacement.h"

#ifdef _WIN32
#include <windows.h>
#endif

using os::Algorithm;
using os::PageReplacement;
using os::PageResult;

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

// 经典例题(教材): 3 页框, FIFO 缺页 15 次, LRU 缺页 12 次
const std::vector<int> kClassicRefs = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3,
                                       0, 3, 2, 1, 2, 0, 1, 7, 0, 1};

// 1. 经典例题: FIFO 与 LRU 缺页次数及缺页率
void testClassic() {
    PageReplacement pr(3, kClassicRefs);
    PageResult fifo = pr.runFIFO();
    PageResult lru = pr.runLRU();
    expect(fifo.faults == 15, "经典例题 FIFO 缺页 15 次");
    expect(lru.faults == 12, "经典例题 LRU 缺页 12 次");
    expect(lru.faultRate() < fifo.faultRate(), "经典例题 LRU 缺页率低于 FIFO");
}

// 2. 全缺页: 页框数小于不同页数且无重复访问, 每次访问均缺页
void testAllFaults() {
    PageReplacement pr(3, {0, 1, 2, 3, 4, 5});
    PageResult fifo = pr.runFIFO();
    PageResult lru = pr.runLRU();
    expect(fifo.faults == 6 && lru.faults == 6, "6 次不同访问均缺页");
    expect(fifo.replacements == 3 && lru.replacements == 3,
           "全缺页时置换 3 次");
    expect(fifo.hits() == 0, "全缺页时命中 0 次");
}

// 3. 无置换: 页框数足以容纳全部页面, 后续访问全部命中
void testNoReplacement() {
    PageReplacement pr(4, {0, 1, 2, 3, 0, 1, 2, 3});
    PageResult r = pr.runFIFO();
    expect(r.faults == 4, "首次装入 4 页后全部命中, 缺页 4 次");
    expect(r.replacements == 0, "无置换");
    expect(r.total == 8 && r.hits() == 4, "命中 4 次");
}

// 4. 命中检测: 页框内重复访问不再缺页
void testHitDetection() {
    PageReplacement pr(2, {0, 1, 0, 1, 0});
    PageResult r = pr.runLRU();
    expect(r.faults == 2, "2 页框重复访问缺页 2 次");
    expect(r.total == 5, "访问总次数 5");
    expect(r.faultRate() == 0.4, "缺页率为 0.4");
}

// 5. 统一入口: run(Algorithm) 与 runFIFO/runLRU 结果一致
void testUnifiedEntry() {
    PageReplacement pr(3, kClassicRefs);
    PageResult a = pr.run(Algorithm::FIFO);
    PageResult b = pr.runFIFO();
    PageResult c = pr.run(Algorithm::LRU);
    PageResult d = pr.runLRU();
    expect(a.faults == b.faults && c.faults == d.faults,
           "run 与便捷方法结果一致");
}

// 6. 构造校验: 页框数非法 / 序列为空 / 页号负数
void testInvalidConstruction() {
    bool threw = false;
    try {
        PageReplacement bad(0, {1, 2, 3});
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    expect(threw, "页框数 0 应抛出异常");

    threw = false;
    try {
        PageReplacement bad(3, {});
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    expect(threw, "空访问序列应抛出异常");

    threw = false;
    try {
        PageReplacement bad(3, {0, -1, 2});
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    expect(threw, "负页号应抛出异常");
}

// 7. 随机序列生成: 长度与取值范围正确
void testRandomRefs() {
    std::vector<int> refs = os::randomRefs(100, 5, 42u);
    expect(refs.size() == 100, "随机序列长度为 100");
    bool inRange = true;
    for (int p : refs)
        if (p < 0 || p >= 5)
            inRange = false;
    expect(inRange, "随机序列页号均在 [0, 5) 内");
}

// 8. 逐步过程记录: 步数等于访问次数, 页框状态长度等于页框数
void testTrace() {
    PageReplacement pr(3, kClassicRefs);
    PageResult r = pr.runFIFO();
    expect(static_cast<int>(r.steps.size()) == r.total,
           "过程步数等于访问总次数");
    bool framesOk = true;
    int faultCount = 0;
    for (const os::PageStep &s : r.steps) {
        if (static_cast<int>(s.frames.size()) != 3)
            framesOk = false;
        if (s.fault)
            ++faultCount;
    }
    expect(framesOk, "每步页框状态长度为 3");
    expect(faultCount == r.faults, "逐步记录中的缺页标记数与缺页次数一致");
}

// 9. formatPercent 格式化
void testFormatPercent() {
    expect(os::formatPercent(0.2344) == "23.44%", "0.2344 -> 23.44%");
    expect(os::formatPercent(0.5) == "50.00%", "0.5 -> 50.00%");
}

} // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::cout << "===== 页面置换算法(FIFO / LRU)局部测试 =====\n";
    testClassic();
    testAllFaults();
    testNoReplacement();
    testHitDetection();
    testUnifiedEntry();
    testInvalidConstruction();
    testRandomRefs();
    testTrace();
    testFormatPercent();

    std::cout << "\n共 " << (g_passed + g_failed) << " 项断言, 通过 "
              << g_passed << " 项, 失败 " << g_failed << " 项\n";
    return g_failed == 0 ? 0 : 1;
}
