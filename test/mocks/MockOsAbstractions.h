#pragma once

#include "platform/IOsAbstractions.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class MockOsAbstractions final : public IOsAbstractions
{
  public:
    void* CreateAndMapMemory(const std::string& Name, std::size_t SizeBytes) override
    {
        m_SharedMemoryName = Name;
        m_AllocatedSizeBytes = SizeBytes;

        const std::size_t WordCount =
            (SizeBytes + sizeof(std::uint64_t) - 1U) / sizeof(std::uint64_t);
        m_AlignedBackingStorage.assign(WordCount, 0U);
        return m_AlignedBackingStorage.data();
    }


    void ReleaseSharedMemory() noexcept override
    {
        m_IsSharedMemoryReleased = true;
    }


    void CreateNotificationSemaphore(const std::string& Name) override
    {
        m_SemaphoreName = Name;
        m_IsSemaphoreCreated = true;
    }


    void NotifyConsumer() override
    {
        ++m_NotificationCount;
    }


    void ReleaseNotificationSemaphore() noexcept override
    {
        m_IsSemaphoreReleased = true;
    }


    std::uint64_t GetMonotonicTimeNs() const override
    {
        return m_CurrentTimeNs;
    }


    void WaitUntilTimeNs(std::uint64_t TargetTimeNs) const override
    {
        m_CurrentTimeNs = TargetTimeNs;
    }


    void RegisterSignalHandlers() override
    {
    }


    bool IsTerminationRequested() const noexcept override
    {
        return false;
    }


    const std::string& GetSharedMemoryName() const
    {
        return m_SharedMemoryName;
    }


    std::size_t GetAllocatedSizeBytes() const
    {
        return m_AllocatedSizeBytes;
    }


    const std::string& GetSemaphoreName() const
    {
        return m_SemaphoreName;
    }


    bool IsSemaphoreCreated() const
    {
        return m_IsSemaphoreCreated;
    }

  private:
    std::vector<std::uint64_t> m_AlignedBackingStorage;
    std::string m_SharedMemoryName;
    std::size_t m_AllocatedSizeBytes = 0;
    bool m_IsSharedMemoryReleased = false;
    std::string m_SemaphoreName;
    bool m_IsSemaphoreCreated = false;
    bool m_IsSemaphoreReleased = false;
    mutable std::uint64_t m_CurrentTimeNs = 0;
    std::size_t m_NotificationCount = 0;
};
