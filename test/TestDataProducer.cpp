#include "config/Config.h"
#include "memory/MemoryHandler.h"
#include "output/BlockAssembler.h"
#include "output/DataProducer.h"
#include "output/Float32SampleQuantizer.h"
#include "output/OutputGenerator.h"
#include "platform/IOsAbstractions.h"
#include "signal/SineSignalGenerator.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace
{
class FakeOsAbstractions final : public IOsAbstractions
{
  public:
    void* CreateAndMapMemory(const std::string&, std::size_t SizeBytes) override
    {
        m_SharedMemory.resize(SizeBytes);
        return m_SharedMemory.data();
    }

    void ReleaseSharedMemory() noexcept override
    {
    }

    void CreateNotificationSemaphore(const std::string&) override
    {
    }

    void NotifyConsumer() override
    {
        ++m_NotificationCount;
    }

    void ReleaseNotificationSemaphore() noexcept override
    {
    }

    std::uint64_t GetMonotonicTimeNs() const override
    {
        return m_CurrentTimeNs;
    }

    void WaitUntilTimeNs(std::uint64_t TargetTimeNs) const override
    {
        m_CurrentTimeNs = TargetTimeNs;
        ++m_WaitCount;
        if (m_WaitCount == 3)
        {
            m_TerminationRequested = true;
        }
    }

    void RegisterSignalHandlers() override
    {
        m_SignalHandlersRegistered = true;
    }

    bool IsTerminationRequested() const noexcept override
    {
        return m_TerminationRequested;
    }

    bool AreSignalHandlersRegistered() const
    {
        return m_SignalHandlersRegistered;
    }

    std::size_t GetNotificationCount() const
    {
        return m_NotificationCount;
    }

  private:
    std::vector<std::byte> m_SharedMemory;
    mutable std::uint64_t m_CurrentTimeNs = 10'000'000ULL;
    mutable std::size_t m_WaitCount = 0;
    mutable bool m_TerminationRequested = false;
    bool m_SignalHandlersRegistered = false;
    std::size_t m_NotificationCount = 0;
};


SimulatorConfig CreateProducerTestConfig()
{
    SimulatorConfig Config;
    Config.FrequencyHz = 50.0;
    Config.NominalAmplitude = 1.0;
    Config.Channels = {ChannelConfig{0, 0.0}};
    Config.SamplesPerCycle = 20;
    Config.SampleRateHz = 1'000.0;
    Config.Representation = SampleRepresentation::Float32;
    Config.Layout = ChannelDataLayout::Interleaved;
    Config.SlotCount = 2;
    Config.BlockSizeSamplesPerChannel = 1;
    Config.SharedMemoryName = "/data_producer_test_memory";
    Config.SemaphoreName = "/data_producer_test_semaphore";
    return Config;
}
} // namespace


TEST_CASE("DataProducer publishes available blocks and drops new blocks when the ring is full",
          "[DataProducer]")
{
    using Catch::Matchers::WithinAbs;

    const SimulatorConfig Config = CreateProducerTestConfig();
    FakeOsAbstractions OsAbstractions;
    MemoryHandler SharedMemory(Config, OsAbstractions);
    SineSignalGenerator SignalGenerator(Config);
    const std::vector<std::unique_ptr<IDisturbanceGenerator>> DisturbanceGenerators;
    OutputGenerator Output(Config, SignalGenerator, DisturbanceGenerators);
    Float32SampleQuantizer Quantizer;
    BlockAssembler Assembler(Config, Output, Quantizer);
    DataProducer Producer(Config, SharedMemory, OsAbstractions, Assembler);

    Producer.Run();

    CHECK(OsAbstractions.AreSignalHandlersRegistered());

    GlobalControlHeader& GlobalHeader = SharedMemory.GetGlobalControlHeader();
    CHECK(GlobalHeader.WriteSeq.load(std::memory_order_acquire) == 2);
    CHECK(GlobalHeader.ReadSeq.load(std::memory_order_acquire) == 0);
    CHECK(GlobalHeader.DroppedBlocks.load(std::memory_order_relaxed) == 1);
    CHECK(OsAbstractions.GetNotificationCount() == 2);

    const PerSlotHeader& FirstSlotHeader = SharedMemory.GetSlotHeader(0);
    CHECK(FirstSlotHeader.BlockSequence.load(std::memory_order_acquire) == 1);
    CHECK(FirstSlotHeader.TimestampNs == 0);
    CHECK(FirstSlotHeader.SampleCountPerChannel == 1);
    CHECK(FirstSlotHeader.PayloadSizeBytes == sizeof(float));

    const PerSlotHeader& SecondSlotHeader = SharedMemory.GetSlotHeader(1);
    CHECK(SecondSlotHeader.BlockSequence.load(std::memory_order_acquire) == 2);
    CHECK(SecondSlotHeader.TimestampNs == 1'000'000ULL);

    float FirstSample = 0.0F;
    std::memcpy(&FirstSample, SharedMemory.GetSlotPayloadStartingAddress(0),
                sizeof(FirstSample));
    CHECK_THAT(FirstSample, WithinAbs(0.0F, 1e-6F));

    float SecondSample = 0.0F;
    std::memcpy(&SecondSample, SharedMemory.GetSlotPayloadStartingAddress(1),
                sizeof(SecondSample));
    CHECK_THAT(SecondSample, WithinAbs(0.30901699F, 1e-6F));
}
