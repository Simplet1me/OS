/**
 * @file banker.h
 * @brief 银行家算法模拟器: 数据结构与类接口定义
 *
 * 银行家算法(Banker's Algorithm)是操作系统中避免死锁的经典算法,
 * 核心思想: 进程申请资源时, 系统先"试探性"分配, 再执行安全性检查,
 * 仅当分配后系统仍处于安全状态时才真正分配, 否则拒绝该申请。
 *
 * 涉及的数据结构(设系统有 n 个进程、m 类资源):
 *   - Available  : 长度为 m 的可用资源向量
 *   - Max        : n×m 矩阵, 进程 i 对资源 j 的最大需求
 *   - Allocation : n×m 矩阵, 进程 i 当前已获得的资源 j 数量
 *   - Need       : n×m 矩阵, 进程 i 尚需的资源 j 数量, 满足 Need = Max - Allocation
 */
#pragma once

#include <string>
#include <vector>

namespace os
{

    /// 资源名: 前 26 类依次为 A~Z, 之后为 R0, R1, ...
    inline std::string resourceName(int index){
        if (index < 26)
            return std::string(1, static_cast<char>('A' + index));
        return "R" + std::to_string(index - 26);
    }

    /**
     * 银行家算法模拟器
     *
     * 典型用法:
     *   Banker b(total, max);            // 初始已分配为 0
     *   Banker b(total, max, alloc);     // 指定初始已分配
     *   b.request(pid, req, &reason);    // 进程申请资源
     *   b.isSafe(&seq);                  // 安全性检查
     */
    class Banker
    {
    public:
        /// 构造: total 为各类资源总量, max 为各进程最大需求, 初始已分配全为 0
        Banker(std::vector<int> total, std::vector<std::vector<int>> max);

        /// 构造: 额外指定各进程初始已分配资源, 可用资源 = 资源总量 - 已分配总量
        /// @throw std::invalid_argument 数据非法(负数、维度不符、超过总量等)时抛出
        Banker(std::vector<int> total, std::vector<std::vector<int>> max,
               std::vector<std::vector<int>> allocation);

        /// 进程 pid 申请 request 数量的各类资源(银行家算法的资源请求算法)
        /// @param pid     进程编号, 取值 [0, processCount)
        /// @param request 长度为 m 的申请向量, 各项非负
        /// @param reason  失败原因输出参数, 可为 nullptr
        /// @return 分配后系统仍安全则同意分配并返回 true;
        ///         否则拒绝申请、回滚试探分配、系统状态不变并返回 false
        bool request(int pid, const std::vector<int> &request, std::string *reason = nullptr);

        /// 安全性检查算法中的一步推导记录
        struct SafeStep
        {
            int pid;                    ///< 进程编号
            std::vector<int> work;      ///< 本步开始时的可用资源(Work)
            std::vector<int> workAfter; ///< 该进程完成后的 Work+Allocation
        };

        /// 安全性检查算法: 判断当前系统是否处于安全状态
        /// @param safeSeq 输出参数: 安全时给出一条安全序列(安全序列一般不唯一), 可为 nullptr
        /// @param steps   输出参数: 逐步推导记录(每步选中的进程与 Work 变化), 可为 nullptr
        bool isSafe(std::vector<int> *safeSeq = nullptr,
                    std::vector<SafeStep> *steps = nullptr) const;

        /// 打印资源分配表(各进程的 Max/Allocation/Need 与 Available)
        void printTable() const;

        /// 打印安全性检查过程表(进程/Max/Available/Need/Allocation/Work+Allocation/Finish)
        /// @return 系统是否安全
        bool printSafetyTable() const;

        // ---- 状态访问器 ----
        int processCount() const { 
            return n_; 
        }

        int resourceCount() const { 
            return m_; 
        }

        const std::vector<std::string> &names() const { 
            return names_; 
        
        }

        const std::vector<int> &available() const { 
            return available_; 
        }

        const std::vector<std::vector<int>> &max() const { 
            return max_; 
        }

        const std::vector<std::vector<int>> &allocation() const { 
            return allocation_; 
        }

        const std::vector<std::vector<int>> &need() const { 
            return need_; 
        }

    private:
        /// 将资源向量格式化为 "(1, 2, 3)" 形式(按所有数值的最大位数统一宽度)
        std::string fmtTuple(const std::vector<int> &v) const;

        int n_; ///< 进程数
        int m_; ///< 资源种类数

        std::vector<std::string> names_;           ///< 进程名(P0, P1, ...)
        std::vector<int> available_;               ///< 可用资源向量
        std::vector<std::vector<int>> max_;        ///< 最大需求矩阵
        std::vector<std::vector<int>> allocation_; ///< 已分配矩阵
        std::vector<std::vector<int>> need_;       ///< 尚需矩阵
    };

} // namespace os
