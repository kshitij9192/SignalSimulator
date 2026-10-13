#include "OsAbstractions.h"

#include <cerrno>
#include <stdexcept>
#include <system_error>

#if defined(__linux__)
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

OsAbstractions::OsAbstractions() = default;


OsAbstractions::~OsAbstractions()
{
    ReleaseSharedMemory();
}


void* OsAbstractions::CreateAndMapMemory(const std::string& rName, std::size_t SizeBytes)
{
    if (m_IsMapped)
    {
        if (m_Name == rName && m_SizeBytes == SizeBytes)
        {
            return m_pMappedAddress;
        }

        throw std::logic_error("shared memory object is already mapped with different parameters");
    }

    if (rName.empty() || rName.find('\0') != std::string::npos)
    {
        throw std::invalid_argument("shared memory name must not be empty or contain null characters");
    }

    if (SizeBytes == 0)
    {
        throw std::invalid_argument("shared memory size must be greater than zero");
    }

#if defined(__linux__)
    if (rName.size() < 2 || rName.front() != '/' || rName.find('/', 1) != std::string::npos)
    {
        throw std::invalid_argument("Linux shared memory name must start with '/' and contain no other '/' characters");
    }

    m_Name = rName;
    m_SizeBytes = SizeBytes;
    m_FileDescriptor = shm_open(rName.c_str(), O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC,
                                S_IRUSR | S_IWUSR);
    if (m_FileDescriptor == -1)
    {
        m_Name.clear();
        m_SizeBytes = 0;
        throw std::system_error(errno, std::generic_category(), "shm_open " + rName);
    }
    m_OwnsName = true;

    if (ftruncate(m_FileDescriptor, static_cast<off_t>(SizeBytes)) == -1)
    {
        const int Error = errno;
        ReleaseSharedMemory();
        throw std::system_error(Error, std::generic_category(), "ftruncate " + rName);
    }

    void* pAddress = mmap(nullptr, SizeBytes, PROT_READ | PROT_WRITE, MAP_SHARED,
                          m_FileDescriptor, 0);
    if (pAddress == MAP_FAILED)
    {
        const int Error = errno;
        ReleaseSharedMemory();
        throw std::system_error(Error, std::generic_category(), "mmap " + rName);
    }

    m_pMappedAddress = pAddress;
    m_IsMapped = true;
    return m_pMappedAddress;
#else
    (void)rName;
    (void)SizeBytes;
    throw std::runtime_error("shared memory is not implemented for this operating system");
#endif
}


void OsAbstractions::ReleaseSharedMemory() noexcept
{
#if defined(__linux__)
    if (m_IsMapped)
    {
        munmap(m_pMappedAddress, m_SizeBytes);
        m_pMappedAddress = nullptr;
        m_IsMapped = false;
    }

    if (m_FileDescriptor != -1)
    {
        close(m_FileDescriptor);
        m_FileDescriptor = -1;
    }

    if (m_OwnsName)
    {
        shm_unlink(m_Name.c_str());
        m_OwnsName = false;
    }
#endif
    m_Name.clear();
    m_SizeBytes = 0;
}
