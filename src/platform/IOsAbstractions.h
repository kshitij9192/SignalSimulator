#pragma once

#include <cstddef>
#include <string>

class IOsAbstractions
{
  public:
    virtual ~IOsAbstractions() = default;

    virtual void* CreateAndMapMemory(const std::string& rName, std::size_t SizeBytes) = 0;
    virtual void ReleaseSharedMemory() noexcept = 0;
};
