#pragma once

#include <cstddef>
#include <string>

class OsAbstractions final
{
  public:
    OsAbstractions();
    ~OsAbstractions();

    OsAbstractions(const OsAbstractions&) = delete;
    OsAbstractions& operator=(const OsAbstractions&) = delete;

    void* CreateAndMapMemory(const std::string& rName, std::size_t SizeBytes);
    void ReleaseSharedMemory() noexcept;

  private:
    std::string m_Name;
    std::size_t m_SizeBytes = 0;
    int m_FileDescriptor = -1;
    void* m_pMappedAddress = nullptr;
    bool m_IsMapped = false;
    bool m_OwnsName = false;
};
