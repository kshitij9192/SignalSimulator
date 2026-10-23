#pragma once

#include "platform/IOsAbstractions.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <semaphore.h>
#include <string>

class LinuxOsAbstractions final : public IOsAbstractions
{
  public:
    LinuxOsAbstractions();
    ~LinuxOsAbstractions() override;

    LinuxOsAbstractions(const LinuxOsAbstractions&) = delete;
    LinuxOsAbstractions& operator=(const LinuxOsAbstractions&) = delete;

    void* CreateAndMapMemory(const std::string& Name, std::size_t SizeBytes) override;
    void ReleaseSharedMemory() noexcept override;
    void CreateNotificationSemaphore(const std::string& Name) override;
    void NotifyConsumer() override;
    void ReleaseNotificationSemaphore() noexcept override;
    std::uint64_t GetMonotonicTimeNs() const override;
    void WaitUntilTimeNs(std::uint64_t TargetTimeNs) const override;
    void RegisterSignalHandlers() override;
    bool IsTerminationRequested() const noexcept override;

  private:
    static void ValidatePosixName(const std::string& Name, const std::string& ResourceLabel);
    static void SleepUntilTimeNs(std::uint64_t SleepTargetTimeNs);
    static void HandleTerminationSignal(int SignalNumber) noexcept;

    static constexpr std::uint64_t m_NanosecondsPerSecond = 1'000'000'000ULL;
    static constexpr std::uint64_t m_SpinGuardTimeNs = 100'000ULL;
    static inline std::atomic<bool> m_TerminationRequested{false};

    std::string m_Name;
    std::size_t m_SizeBytes = 0;
    int m_FileDescriptor = -1;
    void* m_MappedAddress = nullptr;
    bool m_IsMapped = false;
    bool m_OwnsName = false;
    std::string m_SemaphoreName;
    sem_t* m_Semaphore = SEM_FAILED;
    bool m_IsSemaphoreOpen = false;
    bool m_OwnsSemaphoreName = false;
};
