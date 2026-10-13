#pragma once

#include <cstddef>
#include <vector>

namespace os {
using Resources = std::vector<int>;
using ResourceMatrix = std::vector<Resources>;

struct SafetyResult {
    bool safe;
    std::vector<std::size_t> sequence;
};

enum class RequestStatus {
    Granted, InvalidProcess, InvalidResources, ExceedsNeed, InsufficientAvailable, Unsafe
};

struct RequestResult {
    RequestStatus status;
    SafetyResult safety;
};

// Max/Allocation 每行对应一个进程；Available 每列对应一种资源。
class Banker {
public:
    Banker(Resources available, ResourceMatrix maximum, ResourceMatrix allocation);
    const Resources& available() const noexcept;
    const ResourceMatrix& maximum() const noexcept;
    const ResourceMatrix& allocation() const noexcept;
    ResourceMatrix need() const;
    SafetyResult checkSafety() const;
    RequestResult request(std::size_t process, const Resources& resources);

private:
    Resources available_;
    ResourceMatrix maximum_;
    ResourceMatrix allocation_;
};
} // namespace os
