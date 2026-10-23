#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

class IOsAbstractions
{
  public:
    static std::unique_ptr<IOsAbstractions> CreateOsAbstractions();

    virtual ~IOsAbstractions() = default;

    virtual void* CreateAndMapMemory(const std::string& Name, std::size_t SizeBytes) = 0;
    virtual void ReleaseSharedMemory() noexcept = 0;
    virtual void CreateNotificationSemaphore(const std::string& Name) = 0;
    virtual void NotifyConsumer() = 0;
    virtual void ReleaseNotificationSemaphore() noexcept = 0;
    virtual std::uint64_t GetMonotonicTimeNs() const = 0;
    virtual void WaitUntilTimeNs(std::uint64_t TargetTimeNs) const = 0;
    virtual void RegisterSignalHandlers() = 0;
    virtual bool IsTerminationRequested() const noexcept = 0;
};
