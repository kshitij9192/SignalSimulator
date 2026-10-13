#pragma once

#include "OsAbstractions.h"

#include <cstddef>
#include <string>

class MemoryHandler final
{
  public:
    MemoryHandler(std::string Name, std::size_t SizeBytes);
    ~MemoryHandler();

    MemoryHandler(const MemoryHandler&) = delete;
    MemoryHandler& operator=(const MemoryHandler&) = delete;

    void* GetSharedMemory();

  private:
    std::string m_Name;
    std::size_t m_SizeBytes;
    OsAbstractions m_OsAbstractions;
};
