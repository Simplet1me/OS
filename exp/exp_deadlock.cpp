#include "banker.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace {
// 按行读取，避免输入字母或多余数字使后续输入错位；EOF 正常结束。
os::Resources readNumbers(const std::string& prompt, std::size_t count) {
    for (;;) {
        std::cout << prompt << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) throw std::runtime_error("输入结束");
        std::istringstream input(line);
        os::Resources values(count);
        bool valid = true;
        for (auto& value : values)
            if (!(input >> value)) { valid = false; break; }
        input >> std::ws;
        if (valid && input.eof()) return values;
        std::cout << "输入无效，请输入 " << count << " 个整数，用空格分隔。\n";
    }
}

std::string vectorText(const os::Resources& values) {
    std::ostringstream out;
    out << '[';
    for (std::size_t j = 0; j < values.size(); ++j) {
        if (j) out << ' ';
        out << values[j];
    }
    out << ']';
    return out.str();
}

void printTable(const os::Banker& banker) {
    const auto need = banker.need();
    std::size_t width = 12;
    for (std::size_t i = 0; i < need.size(); ++i) {
        for (const auto* row : {&banker.maximum()[i], &banker.allocation()[i], &need[i]})
            if (vectorText(*row).size() + 2 > width) width = vectorText(*row).size() + 2;
    }
    std::cout << "\n资源分配表（向量按 R0, R1, ... 排列）\nAvailable = "
              << vectorText(banker.available()) << '\n';
    const auto w = static_cast<int>(width);
    std::cout << std::left << std::setw(10) << "Process" << std::setw(w) << "Max"
              << std::setw(w) << "Allocation" << std::setw(w) << "Need" << '\n';
    for (std::size_t i = 0; i < need.size(); ++i)
        std::cout << std::setw(10) << ("P" + std::to_string(i))
                  << std::setw(w) << vectorText(banker.maximum()[i])
                  << std::setw(w) << vectorText(banker.allocation()[i])
                  << std::setw(w) << vectorText(need[i]) << '\n';
}

void printSafety(const os::SafetyResult& result) {
    if (!result.safe) {
        std::cout << "系统处于不安全状态，没有完整安全序列。\n";
        return;
    }
    std::cout << "系统处于安全状态。安全序列：";
    for (std::size_t i = 0; i < result.sequence.size(); ++i) {
        if (i) std::cout << " -> ";
        std::cout << 'P' << result.sequence[i];
    }
    std::cout << '\n';
}

os::Banker createBanker() {
    for (;;) {
        const int mode = readNumbers("初始化：1 经典示例，2 自定义数据：", 1)[0];
        if (mode == 1)
            return os::Banker({3, 3, 2}, {{7, 5, 3}, {3, 2, 2}, {9, 0, 2}, {2, 2, 2}, {4, 3, 3}},
                              {{0, 1, 0}, {2, 0, 0}, {3, 0, 2}, {2, 1, 1}, {0, 0, 2}});
        if (mode != 2) { std::cout << "请选择 1 或 2。\n"; continue; }
        const auto sizes = readNumbers("输入进程数和资源种类数（1~100）：", 2);
        if (sizes[0] < 1 || sizes[0] > 100 || sizes[1] < 1 || sizes[1] > 100) {
            std::cout << "数量必须在 1~100 之间。\n"; continue;
        }
        const auto n = static_cast<std::size_t>(sizes[0]);
        const auto m = static_cast<std::size_t>(sizes[1]);
        auto available = readNumbers("输入 Available：", m);
        os::ResourceMatrix maximum(n), allocation(n);
        for (std::size_t i = 0; i < n; ++i) {
            maximum[i] = readNumbers("输入 P" + std::to_string(i) + " 的 Max：", m);
            allocation[i] = readNumbers("输入 P" + std::to_string(i) + " 的 Allocation：", m);
        }
        try { return os::Banker(available, maximum, allocation); }
        catch (const std::invalid_argument& error) {
            std::cout << "初始化失败：" << error.what() << "，请重新初始化。\n";
        }
    }
}
} // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::cout << "银行家算法——死锁避免实验\n";
    try {
        auto banker = createBanker();
        printTable(banker);
        printSafety(banker.checkSafety());
        for (;;) {
            const int process = readNumbers("\n申请进程编号（如 1 表示 P1，-1 退出，-2 查看状态）：", 1)[0];
            if (process == -1) break;
            if (process == -2) { printTable(banker); printSafety(banker.checkSafety()); continue; }
            if (process < 0 || static_cast<std::size_t>(process) >= banker.maximum().size()) {
                std::cout << "不能分配：进程编号无效。\n"; continue;
            }
            const auto resources = readNumbers("输入申请资源向量 Request：", banker.available().size());
            const auto result = banker.request(static_cast<std::size_t>(process), resources);
            switch (result.status) {
            case os::RequestStatus::Granted:
                std::cout << "申请成功，已分配资源。\n";
                printTable(banker); printSafety(result.safety); break;
            case os::RequestStatus::Unsafe:
                std::cout << "不能分配：试分配后系统处于不安全状态，已保持原分配状态。\n"; break;
            case os::RequestStatus::ExceedsNeed:
                std::cout << "不能分配：Request 超过该进程的 Need。\n"; break;
            case os::RequestStatus::InsufficientAvailable:
                std::cout << "不能分配：可用资源不足，进程需等待。\n"; break;
            default:
                std::cout << "不能分配：资源申请无效，数量必须为非负整数。\n"; break;
            }
        }
    } catch (const std::runtime_error&) {
        std::cout << "\n输入结束。\n";
    } catch (const std::exception& error) {
        std::cerr << "程序错误：" << error.what() << '\n';
        return 1;
    }
    return 0;
}
