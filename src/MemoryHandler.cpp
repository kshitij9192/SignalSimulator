#include "MemoryHandler.h"

#include <stdexcept>
#include <utility>

MemoryHandler::MemoryHandler(std::string Name, std::size_t SizeBytes)
    : m_Name(std::move(Name)), m_SizeBytes(SizeBytes)
{
    if (m_SizeBytes == 0)
    {
        throw std::invalid_argument("shared memory size must be greater than zero");
    }
}


MemoryHandler::~MemoryHandler()
{
}


void* MemoryHandler::GetSharedMemory()
{
    return m_OsAbstractions.CreateAndMapMemory(m_Name, m_SizeBytes);
}
