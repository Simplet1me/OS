#include "banker.h"

#include <limits>
#include <stdexcept>
#include <utility>

namespace os
{
    Banker::Banker(Resources available, ResourceMatrix maximum, ResourceMatrix allocation)
        : available_(std::move(available)), maximum_(std::move(maximum)),
          allocation_(std::move(allocation))
    {
        if (available_.empty() || maximum_.empty() || maximum_.size() != allocation_.size())
            throw std::invalid_argument("资源或进程数量无效");
        for (std::size_t j = 0; j < available_.size(); ++j)
        {
            if (available_[j] < 0)
                throw std::invalid_argument("Available 不能为负数");
            long long total = available_[j];
            for (std::size_t i = 0; i < maximum_.size(); ++i)
            {
                if (maximum_[i].size() != available_.size() || allocation_[i].size() != available_.size())
                    throw std::invalid_argument("资源矩阵维度不一致");
                if (allocation_[i][j] < 0 || maximum_[i][j] < allocation_[i][j])
                    throw std::invalid_argument("必须满足 0 <= Allocation <= Max");
                total += allocation_[i][j];
            }
            if (total > std::numeric_limits<int>::max())
                throw std::invalid_argument("资源总量超出 int 范围");
        }
    }

    const Resources &Banker::available() const noexcept { return available_; }
    const ResourceMatrix &Banker::maximum() const noexcept { return maximum_; }
    const ResourceMatrix &Banker::allocation() const noexcept { return allocation_; }

    ResourceMatrix Banker::need() const
    {
        auto result = maximum_;
        for (std::size_t i = 0; i < result.size(); ++i)
            for (std::size_t j = 0; j < available_.size(); ++j)
                result[i][j] -= allocation_[i][j];
        return result;
    }

    SafetyResult Banker::checkSafety() const
    {
        Resources work = available_;
        const auto remaining = need();
        std::vector<bool> finish(maximum_.size(), false);
        SafetyResult result{false, {}};
        bool progress = true;
        while (progress)
        {
            progress = false;
            for (std::size_t i = 0; i < maximum_.size(); ++i)
            {
                if (finish[i])
                    continue;
                bool canFinish = true;
                for (std::size_t j = 0; j < work.size(); ++j)
                    if (remaining[i][j] > work[j])
                        canFinish = false;
                if (!canFinish)
                    continue;
                for (std::size_t j = 0; j < work.size(); ++j)
                    work[j] += allocation_[i][j];
                finish[i] = true;
                result.sequence.push_back(i);
                progress = true;
            }
        }
        result.safe = result.sequence.size() == maximum_.size();
        return result;
    }

    RequestResult Banker::request(std::size_t process, const Resources &resources)
    {
        const auto reject = [](RequestStatus status)
        { return RequestResult{status, {false, {}}}; };
        if (process >= maximum_.size())
            return reject(RequestStatus::InvalidProcess);
        if (resources.size() != available_.size())
            return reject(RequestStatus::InvalidResources);
        for (int value : resources)
            if (value < 0)
                return reject(RequestStatus::InvalidResources);
        for (std::size_t j = 0; j < resources.size(); ++j)
            if (resources[j] > maximum_[process][j] - allocation_[process][j])
                return reject(RequestStatus::ExceedsNeed);
        for (std::size_t j = 0; j < resources.size(); ++j)
            if (resources[j] > available_[j])
                return reject(RequestStatus::InsufficientAvailable);

        // 在副本中试分配；只有安全时才提交，拒绝申请时原状态完全不变。
        Banker candidate = *this;
        for (std::size_t j = 0; j < resources.size(); ++j)
        {
            candidate.available_[j] -= resources[j];
            candidate.allocation_[process][j] += resources[j];
        }
        auto safety = candidate.checkSafety();
        if (!safety.safe)
            return {RequestStatus::Unsafe, std::move(safety)};
        *this = std::move(candidate);
        return {RequestStatus::Granted, std::move(safety)};
    }
} // namespace os
