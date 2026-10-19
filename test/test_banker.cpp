/**
 * @file test_banker.cpp
 * @brief 银行家算法局部测试(自包含, 不引入第三方测试框架)
 *
 * 覆盖: 安全性检查、成功分配、不安全申请拒绝与回滚、各类非法输入。
 * 运行方式: 直接运行本可执行文件, 或通过 ctest 运行。
 */
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "banker.h"

#ifdef _WIN32
#include <windows.h>
#endif

using os::Banker;

namespace {

int g_passed = 0;
int g_failed = 0;

void expect(bool cond, const std::string& what) {
    if (cond) {
        ++g_passed;
        std::cout << "[PASS] " << what << "\n";
    } else {
        ++g_failed;
        std::cout << "[FAIL] " << what << "\n";
    }
}

bool contains(const std::string& text, const std::string& sub) {
    return text.find(sub) != std::string::npos;
}

// ---------------- 经典例题数据(教材示例) ----------------
// 5 个进程、3 类资源 A/B/C, 总量 (10, 5, 7), 初始 Available = (3, 3, 2)
const std::vector<int> kTotal = {10, 5, 7};
const std::vector<std::vector<int>> kMax = {
    {7, 5, 3}, {3, 2, 2}, {9, 0, 2}, {2, 2, 2}, {4, 3, 3}};
const std::vector<std::vector<int>> kAlloc = {
    {0, 1, 0}, {2, 0, 0}, {3, 0, 2}, {2, 1, 1}, {0, 0, 2}};

Banker makeExample() { return Banker(kTotal, kMax, kAlloc); }

/// 通用合法性校验: 按给定序列逐个"执行"进程, 每一步都应满足 Need <= Work,
/// 执行完后 Work += Allocation, 全部执行成功则该序列是一条合法的安全序列
bool validSafeSeq(const Banker& b, const std::vector<int>& seq) {
    if (static_cast<int>(seq.size()) != b.processCount()) return false;
    std::vector<int> work = b.available();
    std::vector<bool> done(b.processCount(), false);
    for (int pid : seq) {
        if (pid < 0 || pid >= b.processCount() || done[pid]) return false;
        for (int j = 0; j < b.resourceCount(); ++j) {
            if (b.need()[pid][j] > work[j]) return false;
        }
        for (int j = 0; j < b.resourceCount(); ++j) work[j] += b.allocation()[pid][j];
        done[pid] = true;
    }
    return true;
}

// 1. 例题初始状态安全性
void testInitialSafe() {
    Banker b = makeExample();
    std::vector<int> seq;
    expect(b.isSafe(&seq), "例题初始状态应安全");
    expect(static_cast<int>(seq.size()) == b.processCount(), "安全序列应包含全部进程");
    expect(validSafeSeq(b, seq), "安全序列经逐步验证合法");
}

// 2. 安全申请: P1 申请 (1, 0, 2), 分配后系统仍安全
void testSafeRequest() {
    Banker b = makeExample();
    std::string reason;
    expect(b.request(1, {1, 0, 2}, &reason), "P1 申请 (1,0,2) 应成功");
    expect(b.available() == std::vector<int>({2, 3, 0}), "分配后 Available 应为 (2,3,0)");
    expect(b.allocation()[1] == std::vector<int>({3, 0, 2}), "分配后 P1 的 Allocation 应为 (3,0,2)");
    expect(b.need()[1] == std::vector<int>({0, 2, 0}), "分配后 P1 的 Need 应为 (0,2,0)");
    std::vector<int> seq;
    expect(b.isSafe(&seq) && validSafeSeq(b, seq), "分配后系统仍安全");
}

// 3. 不安全申请: P0 申请 (0, 3, 0) 会使系统进入不安全状态, 应拒绝并回滚
void testUnsafeRejected() {
    Banker b = makeExample();
    std::string reason;
    expect(!b.request(0, {0, 3, 0}, &reason), "P0 申请 (0,3,0) 应被拒绝");
    expect(contains(reason, "不安全"), "拒绝原因应包含\"不安全\"");
    // 拒绝后系统状态必须保持不变(回滚成功)
    expect(b.available() == std::vector<int>({3, 3, 2}), "拒绝后 Available 未变");
    expect(b.allocation()[0] == std::vector<int>({0, 1, 0}), "拒绝后 P0 的 Allocation 未变");
    expect(b.need()[0] == std::vector<int>({7, 4, 3}), "拒绝后 P0 的 Need 未变");
}

// 4. 超过最大需求: P3 申请 (1, 0, 0), 但 P3 的 Need 为 (0, 1, 1)
void testExceedsNeed() {
    Banker b = makeExample();
    std::string reason;
    expect(!b.request(3, {1, 0, 0}, &reason), "P3 申请 (1,0,0) 超过最大需求应被拒绝");
    expect(contains(reason, "最大需求"), "拒绝原因应包含\"最大需求\"");
}

// 5. 资源不足: P2 申请 (6, 0, 0), 但当前 A 类资源仅剩 3
void testInsufficient() {
    Banker b = makeExample();
    std::string reason;
    expect(!b.request(2, {6, 0, 0}, &reason), "P2 申请 (6,0,0) 因资源不足应被拒绝");
    expect(contains(reason, "不足"), "拒绝原因应包含\"不足\"");
}

// 6. 非法输入: 请求向量长度不符 / 进程编号越界 / 负数申请量
void testIllegalRequest() {
    Banker b = makeExample();
    std::string reason;
    expect(!b.request(0, {1, 0}, &reason), "请求向量长度不符应被拒绝");
    expect(!b.request(5, {0, 0, 0}, &reason), "进程编号越界应被拒绝");
    expect(!b.request(0, {0, -1, 0}, &reason), "负数申请量应被拒绝");
}

// 7. 拒绝不安全申请后, 后续合法申请仍能成功(验证回滚正确性)
void testRollbackThenSucceed() {
    Banker b = makeExample();
    b.request(0, {0, 3, 0});  // 因不安全被拒绝
    expect(b.request(1, {1, 0, 2}), "拒绝不安全申请后, 合法申请仍应成功");
}

// 8. 平凡系统: 1 个进程、1 类资源
void testTrivialSystem() {
    Banker t({1}, {{1}});
    std::vector<int> seq;
    expect(t.isSafe(&seq) && seq == std::vector<int>({0}), "单进程单资源系统应安全");
    expect(t.request(0, {1}), "单进程申请全部资源应成功");
    std::string reason;
    expect(!t.request(0, {1}, &reason), "需求已满足后再次申请应被拒绝");
}

// 9. 构造校验: 非法初始数据应抛出异常
void testInvalidConstruction() {
    bool threw = false;
    try {
        Banker bad({10}, {{11}});  // 最大需求超过资源总量
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    expect(threw, "最大需求超过资源总量应抛出异常");

    threw = false;
    try {
        // P4 的已分配 (9,0,2) 超过其最大需求 (4,3,3)
        Banker bad({10, 5, 7}, kMax,
                   {{0, 1, 0}, {2, 0, 0}, {3, 0, 2}, {2, 1, 1}, {9, 0, 2}});
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    expect(threw, "已分配超过最大需求应抛出异常");

    threw = false;
    try {
        // A 类资源已分配总量 5+2+3+2+0 = 12, 超过总量 10
        Banker bad({10, 5, 7}, kMax,
                   {{5, 3, 2}, {2, 0, 0}, {3, 0, 2}, {2, 1, 1}, {0, 0, 2}});
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    expect(threw, "已分配总量超过资源总量应抛出异常");
}

// 10. 安全性检查推导记录: 步数、Work 变化与每步合法性
void testSafeTrace() {
    Banker b = makeExample();
    std::vector<int> seq;
    std::vector<Banker::SafeStep> steps;
    expect(b.isSafe(&seq, &steps), "安全状态返回 true");
    expect(steps.size() == static_cast<std::size_t>(b.processCount()), "推导步数等于进程数");

    // 每步: 开始时满足 Need <= Work, 结束时 Work+Allocation = Work + Allocation
    bool ok = true;
    for (const Banker::SafeStep& s : steps) {
        for (int j = 0; j < b.resourceCount(); ++j) {
            if (b.need()[s.pid][j] > s.work[j]) ok = false;
            if (s.workAfter[j] != s.work[j] + b.allocation()[s.pid][j]) ok = false;
        }
    }
    expect(ok, "每步推导满足 Need <= Work 且 Work+Allocation = Work + Allocation");
}

}  // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::cout << "===== 银行家算法局部测试 =====\n";
    testInitialSafe();
    testSafeRequest();
    testUnsafeRejected();
    testExceedsNeed();
    testInsufficient();
    testIllegalRequest();
    testRollbackThenSucceed();
    testTrivialSystem();
    testInvalidConstruction();
    testSafeTrace();

    std::cout << "\n共 " << (g_passed + g_failed) << " 项断言, 通过 " << g_passed << " 项, 失败 "
              << g_failed << " 项\n";
    return g_failed == 0 ? 0 : 1;
}
