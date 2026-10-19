/**
 * @file banker.cpp
 * @brief 银行家算法模拟器实现
 *
 * 包含两个核心算法:
 *   1. 资源请求算法 request():
 *        Request > Need      -> 拒绝(超过进程声明的最大需求)
 *        Request > Available -> 拒绝(系统资源不足, 进程只能等待)
 *        试探性分配, 安全性检查通过 -> 正式分配
 *        安全性检查不通过          -> 回滚试探分配并拒绝(会进入不安全状态)
 *   2. 安全性检查算法 isSafe():
 *        Work = Available, 反复寻找 Need <= Work 的未完成进程,
 *        模拟其完成并释放资源(Work += Allocation),
 *        若所有进程都能完成, 则系统安全, 完成的先后顺序即安全序列。
 */
#include "banker.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "table.h"

namespace os {

namespace {

/// 抛出非法参数异常
[[noreturn]] void throwInvalid(const std::string &what) {
    throw std::invalid_argument(what);
}

} // namespace

Banker::Banker(std::vector<int> total, std::vector<std::vector<int>> max)
    : Banker(std::move(total), std::move(max),
             std::vector<std::vector<int>>{}) {}

Banker::Banker(std::vector<int> total, std::vector<std::vector<int>> max,
               std::vector<std::vector<int>> allocation) {
    m_ = static_cast<int>(total.size());
    if (m_ <= 0) {
        throwInvalid("资源种类数必须大于 0");
    }
    n_ = static_cast<int>(max.size());

    if (n_ <= 0) {
        throwInvalid("进程数必须大于 0");
    }

    // ---- 校验输入数据合法性 ----
    for (int v : total) {
        if (v < 0) {
            throwInvalid("资源总量不能为负数");
        }
    }

    for (int i = 0; i < n_; ++i) {
        if (static_cast<int>(max[i].size()) != m_) {
            throwInvalid("进程 " + std::to_string(i) +
                         " 的 Max 向量长度与资源种类数不符");
        }
    }

    if (allocation.empty()) {
        allocation.assign(n_,
                          std::vector<int>(m_, 0)); // 未指定时初始已分配为 0
    } else if (static_cast<int>(allocation.size()) != n_) {
        throwInvalid("Allocation 矩阵行数与进程数不符");
    }
    for (int i = 0; i < n_; ++i) {
        if (static_cast<int>(allocation[i].size()) != m_) {
            throwInvalid("进程 " + std::to_string(i) +
                         " 的 Allocation 向量长度与资源种类数不符");
        }
        for (int j = 0; j < m_; ++j) {
            if (max[i][j] < 0) {
                throwInvalid("进程 " + std::to_string(i) +
                             " 的最大需求不能为负数");
            }
            if (max[i][j] > total[j]) {
                throwInvalid("进程 " + std::to_string(i) + " 对资源 " +
                             resourceName(j) + " 的最大需求超过资源总量");
            }
            if (allocation[i][j] < 0) {
                throwInvalid("进程 " + std::to_string(i) +
                             " 的已分配资源不能为负数");
            }
            if (allocation[i][j] > max[i][j]) {
                throwInvalid("进程 " + std::to_string(i) +
                             " 的已分配资源超过其最大需求");
            }
        }
    }
    std::vector<int> sum(m_, 0);
    for (int i = 0; i < n_; ++i) {
        for (int j = 0; j < m_; ++j) {
            sum[j] += allocation[i][j];
        }
    }
    for (int j = 0; j < m_; ++j) {
        if (sum[j] > total[j]) {
            throwInvalid("资源 " + resourceName(j) +
                         " 的已分配总量超过资源总量");
        }
    }

    // ---- 初始化系统状态 ----
    max_ = std::move(max);
    allocation_ = std::move(allocation);
    available_.resize(m_);
    for (int j = 0; j < m_; ++j) {
        available_[j] = total[j] - sum[j];
    }

    need_.assign(n_, std::vector<int>(m_, 0));
    for (int i = 0; i < n_; ++i) {
        for (int j = 0; j < m_; ++j) {
            need_[i][j] = max_[i][j] - allocation_[i][j];
        }
    }

    names_.resize(n_);
    for (int i = 0; i < n_; ++i) {
        names_[i] = "P" + std::to_string(i);
    }
}

// 资源请求算法
bool Banker::request(int pid, const std::vector<int> &request,
                     std::string *reason) {
    auto fail = [reason](const std::string &msg) {
        if (reason) {
            *reason = msg;
        }
        return false;
    };

    // (0) 基本合法性检查
    if (pid < 0 || pid >= n_) {
        return fail("进程编号无效");
    }
    if (static_cast<int>(request.size()) != m_) {
        return fail("请求向量长度与资源种类数不符");
    }
    for (int j = 0; j < m_; ++j) {
        if (request[j] < 0) {
            return fail("申请的资源数量不能为负数");
        }
    }

    // (1) Request <= Need ? 否则进程错误地要求超过其声明的最大需求
    for (int j = 0; j < m_; ++j) {
        if (request[j] > need_[pid][j]) {
            return fail("请求超过进程 " + names_[pid] + " 声明的最大需求");
        }
    }
    // (2) Request <= Available ? 否则系统当前资源不足, 进程只能等待
    for (int j = 0; j < m_; ++j) {
        if (request[j] > available_[j]) {
            return fail("当前可用资源不足, 进程 " + names_[pid] + " 需等待");
        }
    }

    // (3) 试探性分配
    for (int j = 0; j < m_; ++j) {
        available_[j] -= request[j];
        allocation_[pid][j] += request[j];
        need_[pid][j] -= request[j];
    }

    // (4) 安全性检查: 分配后仍安全则同意分配
    if (isSafe()) {
        return true;
    }

    // (5) 不安全: 回滚试探分配, 拒绝申请
    for (int j = 0; j < m_; ++j) {
        available_[j] += request[j];
        allocation_[pid][j] -= request[j];
        need_[pid][j] += request[j];
    }
    return fail("若满足该请求, 系统将进入不安全状态, 可能发生死锁");
}

bool Banker::isSafe(std::vector<int> *safeSeq,
                    std::vector<SafeStep> *steps) const {
    std::vector<int> work = available_; // 工作向量, 初始为当前可用资源
    std::vector<bool> finish(n_, false);
    std::vector<int> seq;

    // 反复寻找"尚需资源 <= 当前可用资源"的未完成进程,
    // 找到则模拟其完成并释放全部已分配资源, 最多进行 n 轮
    for (int round = 0; round < n_; ++round) {
        int chosen = -1;
        for (int i = 0; i < n_; ++i) {
            if (finish[i]) {
                continue;
            }
            bool satisfied = true; // 判断 Need[i] <= Work ?
            for (int j = 0; j < m_; ++j) {
                if (need_[i][j] > work[j]) {
                    satisfied = false;
                    break;
                }
            }
            if (satisfied) {
                chosen = i;
                break;
            }
        }
        if (chosen < 0) {
            break; // 本轮找不到可完成的进程, 提前结束
        }
        SafeStep step;
        step.pid = chosen;
        step.work = work; // 本步开始时的 Work
        for (int j = 0; j < m_; ++j) {
            work[j] += allocation_[chosen][j]; // 进程完成后释放资源
        }
        step.workAfter = work; // 完成后的 Work+Allocation
        if (steps) {
            steps->push_back(std::move(step));
        }
        finish[chosen] = true;
        seq.push_back(chosen);
    }

    bool safe =
        std::all_of(finish.begin(), finish.end(), [](bool f) { return f; });
    if (safe && safeSeq) {
        *safeSeq = std::move(seq);
    }
    return safe;
}

std::string Banker::fmtTuple(const std::vector<int> &v) const {
    // 根据所有数值的最大位数确定每项宽度, 保证元组对齐
    int digits = 1;
    auto bump = [&digits](int x) {
        if (x < 0) {
            x = -x;
        }
        int d = 1;
        while (x >= 10) {
            ++d;
            x /= 10;
        }
        if (d > digits) {
            digits = d;
        }
    };
    for (int val : available_) {
        bump(val);
    }
    for (int i = 0; i < n_; ++i) {
        for (int j = 0; j < m_; ++j) {
            bump(max_[i][j]);
            bump(allocation_[i][j]);
            bump(need_[i][j]);
        }
    }

    std::ostringstream oss;
    oss << "(";
    for (int j = 0; j < m_; ++j) {
        if (j > 0) {
            oss << ", ";
        }
        oss << std::right << std::setw(digits) << v[j];
    }
    oss << ")";
    return oss.str();
}

void Banker::printTable() const {
    // 用通用表格工具类打印资源分配表
    Table table({"进程", "Max", "Allocation", "Need"});
    table.setTitle("资源分配表 (进程数: " + std::to_string(n_) +
                   ", 资源种类数: " + std::to_string(m_) + ")");
    for (int i = 0; i < n_; ++i) {
        table.addRow({names_[i], fmtTuple(max_[i]), fmtTuple(allocation_[i]),
                      fmtTuple(need_[i])});
    }
    table.print();

    // 可用资源行(表格之外单独打印)
    std::ostringstream res;
    for (int j = 0; j < m_; ++j) {
        if (j > 0) {
            res << ", ";
        }
        res << resourceName(j);
    }
    std::cout << "Available(" << res.str() << ") = " << fmtTuple(available_)
              << "\n";
}

bool Banker::printSafetyTable() const {
    // 执行安全性检查, 记录逐步推导过程
    std::vector<int> seq;
    std::vector<SafeStep> steps;
    bool safe = isSafe(&seq, &steps);

    // 过程表: 每行 = 推导中的一步(选中的进程及其 Work 变化)
    Table table({"进程", "Max", "Available", "Need", "Allocation",
                 "Work+Allocation", "Finish"});
    table.setTitle("安全性检查过程 (银行家算法)");
    for (const SafeStep &s : steps) {
        table.addRow({names_[s.pid], fmtTuple(max_[s.pid]), fmtTuple(s.work),
                      fmtTuple(need_[s.pid]), fmtTuple(allocation_[s.pid]),
                      fmtTuple(s.workAfter), "true"});
    }
    // 算法提前终止时(不安全), 将未能完成的进程补在表尾, Finish = false
    if (!safe) {
        std::vector<int> work =
            steps.empty() ? available_ : steps.back().workAfter;
        for (int i = 0; i < n_; ++i) {
            bool done = false;
            for (const SafeStep &s : steps) {
                if (s.pid == i) {
                    done = true;
                    break;
                }
            }
            if (!done) {
                table.addRow({names_[i], fmtTuple(max_[i]), fmtTuple(work),
                              fmtTuple(need_[i]), fmtTuple(allocation_[i]), "-",
                              "false"});
            }
        }
    }
    table.print();

    // 结论: 安全则给出一条安全序列, 否则给出死锁风险提示
    if (safe) {
        std::cout << "系统处于[安全状态], 一条安全序列: ";
        for (std::size_t k = 0; k < seq.size(); ++k) {
            if (k > 0) {
                std::cout << " -> ";
            }
            std::cout << names_[seq[k]];
        }
        std::cout << "\n";
    } else {
        std::cout << "系统处于[不安全状态], 不存在安全序列, 可能发生死锁!\n";
    }
    return safe;
}

} // namespace os
