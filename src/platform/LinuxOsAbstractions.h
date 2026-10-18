#pragma once

#include "platform/IOsAbstractions.h"

#include <cstddef>
#include <string>

class LinuxOsAbstractions final : public IOsAbstractions
{
  public:
    LinuxOsAbstractions();
    ~LinuxOsAbstractions() override;

    LinuxOsAbstractions(const LinuxOsAbstractions&) = delete;
    LinuxOsAbstractions& operator=(const LinuxOsAbstractions&) = delete;

    void* CreateAndMapMemory(const std::string& rName, std::size_t SizeBytes) override;
    void ReleaseSharedMemory() noexcept override;

  private:
    std::string m_Name;
    std::size_t m_SizeBytes = 0;
    int m_FileDescriptor = -1;
    void* m_pMappedAddress = nullptr;
    bool m_IsMapped = false;
    bool m_OwnsName = false;
};
