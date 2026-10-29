#include "platform/LinuxOsAbstractions.h"

#include <cerrno>
#include <csignal>
#include <ctime>
#include <fcntl.h>
#include <memory>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

std::unique_ptr<IOsAbstractions> IOsAbstractions::CreateOsAbstractions()
{
    return std::make_unique<LinuxOsAbstractions>();
}


LinuxOsAbstractions::LinuxOsAbstractions()
{
    m_TerminationRequested.store(false, std::memory_order_relaxed);
}


LinuxOsAbstractions::~LinuxOsAbstractions()
{
    ReleaseNotificationSemaphore();
    ReleaseSharedMemory();
}


void LinuxOsAbstractions::ValidatePosixName(const std::string& Name,
                                            const std::string& ResourceLabel)
{
    if (Name.empty() || Name.find('\0') != std::string::npos)
    {
        throw std::invalid_argument(ResourceLabel +
                                    " name must not be empty or contain null characters");
    }

    if (Name.size() < 2 || Name.front() != '/' || Name.find('/', 1) != std::string::npos)
    {
        throw std::invalid_argument("Linux " + ResourceLabel +
                                    " name must start with '/' and contain no other '/' characters");
    }
}


void* LinuxOsAbstractions::CreateAndMapMemory(const std::string& Name, std::size_t SizeBytes)
{
    if (m_IsMapped)
    {
        if (m_Name != Name || m_SizeBytes != SizeBytes)
        {
            throw std::logic_error(
                "shared memory object is already mapped with different parameters");
        }
    }
    else
    {
        ValidatePosixName(Name, "shared memory");

        if (SizeBytes == 0)
        {
            throw std::invalid_argument("shared memory size must be greater than zero");
        }

        m_Name = Name;
        m_SizeBytes = SizeBytes;
        m_FileDescriptor = shm_open(Name.c_str(), O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, S_IRUSR | S_IWUSR);
        if (m_FileDescriptor == -1)
        {
            m_Name.clear();
            m_SizeBytes = 0;
            throw std::system_error(errno, std::generic_category(), "shm_open " + Name);
        }
        m_OwnsName = true;

        if (ftruncate(m_FileDescriptor, static_cast<off_t>(SizeBytes)) == -1)
        {
            const int Error = errno;
            ReleaseSharedMemory();
            throw std::system_error(Error, std::generic_category(), "ftruncate " + Name);
        }

        void* Address = mmap(nullptr, SizeBytes, PROT_READ | PROT_WRITE, MAP_SHARED, m_FileDescriptor, 0);
        if (Address == MAP_FAILED)
        {
            const int Error = errno;
            ReleaseSharedMemory();
            throw std::system_error(Error, std::generic_category(), "mmap " + Name);
        }

        m_MappedAddress = Address;
        m_IsMapped = true;
    }

    return m_MappedAddress;
}


void LinuxOsAbstractions::ReleaseSharedMemory() noexcept
{
    if (m_IsMapped)
    {
        munmap(m_MappedAddress, m_SizeBytes);
        m_MappedAddress = nullptr;
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

    m_Name.clear();
    m_SizeBytes = 0;
}


void LinuxOsAbstractions::CreateNotificationSemaphore(const std::string& Name)
{
    if (m_IsSemaphoreOpen)
    {
        if (m_SemaphoreName != Name)
        {
            throw std::logic_error(
                "notification semaphore is already open with different parameters");
        }
    }
    else
    {
        ValidatePosixName(Name, "semaphore");

        sem_t* Semaphore = sem_open(Name.c_str(), O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0U);
        if (Semaphore == SEM_FAILED)
        {
            throw std::system_error(errno, std::generic_category(), "sem_open " + Name);
        }

        m_SemaphoreName = Name;
        m_Semaphore = Semaphore;
        m_IsSemaphoreOpen = true;
        m_OwnsSemaphoreName = true;
    }
}


void LinuxOsAbstractions::NotifyConsumer()
{
    if (!m_IsSemaphoreOpen || m_Semaphore == SEM_FAILED)
    {
        throw std::logic_error("notification semaphore is not initialized");
    }

    if (sem_post(m_Semaphore) == -1)
    {
        throw std::system_error(errno, std::generic_category(), "sem_post " + m_SemaphoreName);
    }
}


void LinuxOsAbstractions::ReleaseNotificationSemaphore() noexcept
{
    if (m_IsSemaphoreOpen && m_Semaphore != SEM_FAILED)
    {
        sem_close(m_Semaphore);
        m_Semaphore = SEM_FAILED;
        m_IsSemaphoreOpen = false;
    }

    if (m_OwnsSemaphoreName)
    {
        sem_unlink(m_SemaphoreName.c_str());
        m_OwnsSemaphoreName = false;
    }

    m_SemaphoreName.clear();
}


void LinuxOsAbstractions::SleepUntilTimeNs(std::uint64_t SleepTargetTimeNs)
{
    struct timespec TargetSpec
    {
    };
    TargetSpec.tv_sec = static_cast<time_t>(SleepTargetTimeNs / m_NanosecondsPerSecond);
    TargetSpec.tv_nsec = static_cast<long>(SleepTargetTimeNs % m_NanosecondsPerSecond);

    const int Result = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &TargetSpec, nullptr);
    if (Result != 0 && Result != EINTR)
    {
        throw std::system_error(Result, std::generic_category(), "clock_nanosleep");
    }
}


std::uint64_t LinuxOsAbstractions::GetMonotonicTimeNs() const
{
    struct timespec CurrentTime
    {
    };
    if (clock_gettime(CLOCK_MONOTONIC, &CurrentTime) == -1)
    {
        throw std::system_error(errno, std::generic_category(), "clock_gettime");
    }

    return (static_cast<std::uint64_t>(CurrentTime.tv_sec) * m_NanosecondsPerSecond) +
           static_cast<std::uint64_t>(CurrentTime.tv_nsec);
}


void LinuxOsAbstractions::WaitUntilTimeNs(std::uint64_t TargetTimeNs) const
{
    const std::uint64_t CurrentTimeNs = GetMonotonicTimeNs();
    if (TargetTimeNs > CurrentTimeNs)
    {
        if ((TargetTimeNs - CurrentTimeNs) > m_SpinGuardTimeNs)
        {
            SleepUntilTimeNs(TargetTimeNs - m_SpinGuardTimeNs);
        }

        while (!IsTerminationRequested() && GetMonotonicTimeNs() < TargetTimeNs)
        {
        }
    }
}


void LinuxOsAbstractions::HandleTerminationSignal(int SignalNumber) noexcept
{
    if (SignalNumber == SIGINT || SignalNumber == SIGTERM)
    {
        m_TerminationRequested.store(true, std::memory_order_relaxed);
    }
}


void LinuxOsAbstractions::RegisterSignalHandlers()
{
    m_TerminationRequested.store(false, std::memory_order_relaxed);

    struct sigaction SignalAction
    {
    };

    SignalAction.sa_handler = &LinuxOsAbstractions::HandleTerminationSignal;
    sigemptyset(&SignalAction.sa_mask);
    SignalAction.sa_flags = 0;

    if (sigaction(SIGINT, &SignalAction, nullptr) == -1)
    {
        throw std::system_error(errno, std::generic_category(), "sigaction SIGINT");
    }

    if (sigaction(SIGTERM, &SignalAction, nullptr) == -1)
    {
        throw std::system_error(errno, std::generic_category(), "sigaction SIGTERM");
    }
}


bool LinuxOsAbstractions::IsTerminationRequested() const noexcept
{
    return m_TerminationRequested.load(std::memory_order_relaxed);
}
