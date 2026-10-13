#include "banker.h"
#include <iostream>
#include <stdexcept>

namespace
{
    void require(bool condition, const char *message)
    {
        if (!condition)
            throw std::runtime_error(message);
    }
    os::Banker sample()
    {
        return os::Banker({3, 3, 2}, {{7, 5, 3}, {3, 2, 2}, {9, 0, 2}, {2, 2, 2}, {4, 3, 3}},
                          {{0, 1, 0}, {2, 0, 0}, {3, 0, 2}, {2, 1, 1}, {0, 0, 2}});
    }
    void verifySequence(const os::Banker &banker, const os::SafetyResult &result)
    {
        require(result.safe, "expected safe state");
        auto work = banker.available();
        const auto need = banker.need();
        std::vector<bool> visited(need.size(), false);
        require(result.sequence.size() == need.size(), "incomplete sequence");
        for (auto p : result.sequence)
        {
            require(p < need.size() && !visited[p], "invalid sequence process");
            visited[p] = true;
            for (std::size_t j = 0; j < work.size(); ++j)
            {
                require(need[p][j] <= work[j], "process cannot finish");
                work[j] += banker.allocation()[p][j];
            }
        }
    }
}
int main()
{
    try
    {
        auto banker = sample();
        verifySequence(banker, banker.checkSafety());
        const auto originalAvailable = banker.available();
        const auto originalAllocation = banker.allocation();
        require(banker.request(4, {3, 3, 0}).status == os::RequestStatus::Unsafe, "unsafe request accepted");
        require(banker.available() == originalAvailable && banker.allocation() == originalAllocation,
                "unsafe request changed state");
        require(banker.request(0, {0, 4, 0}).status == os::RequestStatus::InsufficientAvailable, "availability check");
        require(banker.request(1, {2, 0, 0}).status == os::RequestStatus::ExceedsNeed, "need check");
        require(banker.request(5, {0, 0, 0}).status == os::RequestStatus::InvalidProcess, "process check");
        require(banker.request(1, {-1, 0, 0}).status == os::RequestStatus::InvalidResources, "negative check");
        require(banker.request(1, {0}).status == os::RequestStatus::InvalidResources, "dimension check");
        require(banker.available() == originalAvailable && banker.allocation() == originalAllocation,
                "rejected request changed state");
        const auto granted = banker.request(1, {1, 0, 2});
        require(granted.status == os::RequestStatus::Granted, "safe request rejected");
        require(banker.available() == os::Resources({2, 3, 0}), "incorrect available");
        require(banker.allocation()[1] == os::Resources({3, 0, 2}), "incorrect allocation");
        require(banker.need()[1] == os::Resources({0, 2, 0}), "incorrect need");
        verifySequence(banker, granted.safety);
        verifySequence(banker, banker.request(3, {0, 1, 0}).safety);
        os::Banker revisit({0}, {{2}, {1}}, {{1}, {1}});
        verifySequence(revisit, revisit.checkSafety());
        os::Banker unsafe({0}, {{2}, {2}}, {{1}, {1}});
        require(!unsafe.checkSafety().safe, "unsafe initial state");
        os::Banker finished({0}, {{1}}, {{1}});
        verifySequence(finished, finished.checkSafety());
        int invalidCount = 0;
        for (int scenario = 0; scenario < 5; ++scenario)
        {
            try
            {
                if (scenario == 0)
                {
                    os::Banker b({}, {}, {});
                }
                if (scenario == 1)
                {
                    os::Banker b({1}, {{1, 2}}, {{0}});
                }
                if (scenario == 2)
                {
                    os::Banker b({-1}, {{1}}, {{0}});
                }
                if (scenario == 3)
                {
                    os::Banker b({1}, {{1}}, {{2}});
                }
                if (scenario == 4)
                {
                    os::Banker b({2147483647}, {{1}}, {{1}});
                }
            }
            catch (const std::invalid_argument &)
            {
                ++invalidCount;
            }
        }
        require(invalidCount == 5, "invalid initialization accepted");
        std::cout << "All banker tests passed.\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
