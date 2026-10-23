#include "platform/IOsAbstractions.h"
#include "platform/LinuxOsAbstractions.h"

#include <catch2/catch_test_macros.hpp>

#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <semaphore.h>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>

class LinuxOsAbstractionsTestFixture
{
  protected:
    LinuxOsAbstractionsTestFixture()
        : m_SharedMemoryName("/sigsim_test_shm_" + std::to_string(getpid())),
          m_SemaphoreName("/sigsim_test_sem_" + std::to_string(getpid()))
    {
        shm_unlink(m_SharedMemoryName.c_str());
        sem_unlink(m_SemaphoreName.c_str());
    }


    ~LinuxOsAbstractionsTestFixture()
    {
        shm_unlink(m_SharedMemoryName.c_str());
        sem_unlink(m_SemaphoreName.c_str());
    }

    std::string m_SharedMemoryName;
    std::string m_SemaphoreName;
    LinuxOsAbstractions m_OsAbstractions;
};


TEST_CASE_METHOD(LinuxOsAbstractionsTestFixture,
                 "LinuxOsAbstractions manages POSIX shared memory lifecycle",
                 "[LinuxOsAbstractions][SharedMemory]")
{
    SECTION("Creates, maps, writes, and allows consumer to read shared memory")
    {
        constexpr std::size_t MemorySize = 128;
        void* MappedAddress =
            m_OsAbstractions.CreateAndMapMemory(m_SharedMemoryName, MemorySize);
        REQUIRE(MappedAddress != nullptr);

        constexpr std::uint64_t TestPattern = 0x1122334455667788ULL;
        std::memcpy(MappedAddress, &TestPattern, sizeof(TestPattern));

        const int ConsumerFd = shm_open(m_SharedMemoryName.c_str(), O_RDONLY, 0);
        REQUIRE(ConsumerFd != -1);

        struct stat ShmStat
        {
        };
        REQUIRE(fstat(ConsumerFd, &ShmStat) == 0);
        CHECK(static_cast<std::size_t>(ShmStat.st_size) == MemorySize);

        void* ConsumerAddress =
            mmap(nullptr, MemorySize, PROT_READ, MAP_SHARED, ConsumerFd, 0);
        REQUIRE(ConsumerAddress != MAP_FAILED);

        std::uint64_t ReadBackPattern = 0;
        std::memcpy(&ReadBackPattern, ConsumerAddress, sizeof(ReadBackPattern));
        CHECK(ReadBackPattern == TestPattern);

        munmap(ConsumerAddress, MemorySize);
        close(ConsumerFd);
    }

    SECTION("Calling CreateAndMapMemory with identical parameters returns the same address")
    {
        constexpr std::size_t MemorySize = 64;
        void* FirstAddress =
            m_OsAbstractions.CreateAndMapMemory(m_SharedMemoryName, MemorySize);
        void* SecondAddress =
            m_OsAbstractions.CreateAndMapMemory(m_SharedMemoryName, MemorySize);

        CHECK(FirstAddress == SecondAddress);
    }

    SECTION("Calling CreateAndMapMemory with different parameters while mapped throws logic_error")
    {
        m_OsAbstractions.CreateAndMapMemory(m_SharedMemoryName, 64);

        CHECK_THROWS_AS(m_OsAbstractions.CreateAndMapMemory(m_SharedMemoryName, 128),
                        std::logic_error);
        CHECK_THROWS_AS(
            m_OsAbstractions.CreateAndMapMemory(m_SharedMemoryName + "_other", 64),
            std::logic_error);
    }

    SECTION("Creating duplicate shared memory with the same name fails with system_error")
    {
        m_OsAbstractions.CreateAndMapMemory(m_SharedMemoryName, 64);

        LinuxOsAbstractions SecondInstance;
        CHECK_THROWS_AS(SecondInstance.CreateAndMapMemory(m_SharedMemoryName, 64),
                        std::system_error);
    }

    SECTION("ReleaseSharedMemory unlinks the shared memory object and allows recreation")
    {
        m_OsAbstractions.CreateAndMapMemory(m_SharedMemoryName, 64);
        m_OsAbstractions.ReleaseSharedMemory();

        const int ConsumerFd = shm_open(m_SharedMemoryName.c_str(), O_RDONLY, 0);
        CHECK(ConsumerFd == -1);
        CHECK(errno == ENOENT);

        void* RecreatedAddress =
            m_OsAbstractions.CreateAndMapMemory(m_SharedMemoryName, 64);
        CHECK(RecreatedAddress != nullptr);
    }

    SECTION("Rejects invalid shared memory names and zero size")
    {
        CHECK_THROWS_AS(m_OsAbstractions.CreateAndMapMemory("", 64), std::invalid_argument);
        CHECK_THROWS_AS(m_OsAbstractions.CreateAndMapMemory("no_leading_slash", 64),
                        std::invalid_argument);
        CHECK_THROWS_AS(m_OsAbstractions.CreateAndMapMemory("/", 64), std::invalid_argument);
        CHECK_THROWS_AS(m_OsAbstractions.CreateAndMapMemory("/nested/slash", 64),
                        std::invalid_argument);
        CHECK_THROWS_AS(m_OsAbstractions.CreateAndMapMemory(m_SharedMemoryName, 0),
                        std::invalid_argument);
    }
}


TEST_CASE_METHOD(LinuxOsAbstractionsTestFixture,
                 "LinuxOsAbstractions manages POSIX named semaphore lifecycle",
                 "[LinuxOsAbstractions][Semaphore]")
{
    SECTION("Creates semaphore initialized to zero and signals consumer via NotifyConsumer")
    {
        m_OsAbstractions.CreateNotificationSemaphore(m_SemaphoreName);

        sem_t* ConsumerSemaphore = sem_open(m_SemaphoreName.c_str(), 0);
        REQUIRE(ConsumerSemaphore != SEM_FAILED);

        CHECK(sem_trywait(ConsumerSemaphore) == -1);
        CHECK(errno == EAGAIN);

        m_OsAbstractions.NotifyConsumer();
        CHECK(sem_trywait(ConsumerSemaphore) == 0);

        CHECK(sem_trywait(ConsumerSemaphore) == -1);
        CHECK(errno == EAGAIN);

        sem_close(ConsumerSemaphore);
    }

    SECTION("Calling CreateNotificationSemaphore with identical name is idempotent")
    {
        m_OsAbstractions.CreateNotificationSemaphore(m_SemaphoreName);
        CHECK_NOTHROW(m_OsAbstractions.CreateNotificationSemaphore(m_SemaphoreName));

        CHECK_THROWS_AS(
            m_OsAbstractions.CreateNotificationSemaphore(m_SemaphoreName + "_other"),
            std::logic_error);
    }

    SECTION("NotifyConsumer throws logic_error when semaphore is not open")
    {
        CHECK_THROWS_AS(m_OsAbstractions.NotifyConsumer(), std::logic_error);

        m_OsAbstractions.CreateNotificationSemaphore(m_SemaphoreName);
        m_OsAbstractions.ReleaseNotificationSemaphore();

        CHECK_THROWS_AS(m_OsAbstractions.NotifyConsumer(), std::logic_error);
    }

    SECTION("ReleaseNotificationSemaphore unlinks the semaphore and allows recreation")
    {
        m_OsAbstractions.CreateNotificationSemaphore(m_SemaphoreName);
        m_OsAbstractions.ReleaseNotificationSemaphore();

        sem_t* ConsumerSemaphore = sem_open(m_SemaphoreName.c_str(), 0);
        CHECK(ConsumerSemaphore == SEM_FAILED);
        CHECK(errno == ENOENT);

        CHECK_NOTHROW(m_OsAbstractions.CreateNotificationSemaphore(m_SemaphoreName));
    }

    SECTION("Rejects invalid semaphore names")
    {
        CHECK_THROWS_AS(m_OsAbstractions.CreateNotificationSemaphore(""),
                        std::invalid_argument);
        CHECK_THROWS_AS(m_OsAbstractions.CreateNotificationSemaphore("missing_slash"),
                        std::invalid_argument);
        CHECK_THROWS_AS(m_OsAbstractions.CreateNotificationSemaphore("/"),
                        std::invalid_argument);
        CHECK_THROWS_AS(m_OsAbstractions.CreateNotificationSemaphore("/extra/slash"),
                        std::invalid_argument);
    }

    SECTION("Factory method creates a valid IOsAbstractions instance")
    {
        const std::unique_ptr<IOsAbstractions> Instance =
            IOsAbstractions::CreateOsAbstractions();
        REQUIRE(Instance != nullptr);
    }
}


TEST_CASE_METHOD(LinuxOsAbstractionsTestFixture,
                 "LinuxOsAbstractions provides monotonic time and deadline pacing",
                 "[LinuxOsAbstractions][Timer]")
{
    SECTION("GetMonotonicTimeNs returns non-decreasing timestamps")
    {
        const std::uint64_t FirstTimestampNs = m_OsAbstractions.GetMonotonicTimeNs();
        const std::uint64_t SecondTimestampNs = m_OsAbstractions.GetMonotonicTimeNs();

        CHECK(FirstTimestampNs > 0ULL);
        CHECK(SecondTimestampNs >= FirstTimestampNs);
    }

    SECTION("WaitUntilTimeNs returns immediately for past or current deadlines")
    {
        const std::uint64_t StartTimeNs = m_OsAbstractions.GetMonotonicTimeNs();
        m_OsAbstractions.WaitUntilTimeNs(StartTimeNs);
        const std::uint64_t AfterWaitNs = m_OsAbstractions.GetMonotonicTimeNs();

        CHECK(AfterWaitNs >= StartTimeNs);
    }

    SECTION("WaitUntilTimeNs waits at least until future target deadline")
    {
        constexpr std::uint64_t WaitDurationNs = 2'000'000ULL;
        const std::uint64_t TargetDeadlineNs =
            m_OsAbstractions.GetMonotonicTimeNs() + WaitDurationNs;

        m_OsAbstractions.WaitUntilTimeNs(TargetDeadlineNs);
        const std::uint64_t WakeTimeNs = m_OsAbstractions.GetMonotonicTimeNs();

        CHECK(WakeTimeNs >= TargetDeadlineNs);
    }

    SECTION("RegisterSignalHandlers handles SIGINT and SIGTERM and sets termination flag")
    {
        m_OsAbstractions.RegisterSignalHandlers();
        CHECK_FALSE(m_OsAbstractions.IsTerminationRequested());

        REQUIRE(raise(SIGINT) == 0);
        CHECK(m_OsAbstractions.IsTerminationRequested());

        m_OsAbstractions.RegisterSignalHandlers();
        CHECK_FALSE(m_OsAbstractions.IsTerminationRequested());

        REQUIRE(raise(SIGTERM) == 0);
        CHECK(m_OsAbstractions.IsTerminationRequested());
    }
}
