#include "output/DataProducer.h"

#include "memory/MemoryHandler.h"
#include "output/BlockAssembler.h"
#include "platform/IOsAbstractions.h"

#include <cmath>
#include <limits>
#include <stdexcept>

DataProducer::DataProducer(const SimulatorConfig& Config, MemoryHandler& MemoryHandlerInstance,
                           IOsAbstractions& OsAbstractions, BlockAssembler& BlockAssemblerInstance)
    : m_Config(Config),
      m_MemoryHandler(MemoryHandlerInstance),
      m_OsAbstractions(OsAbstractions),
      m_BlockAssembler(BlockAssemblerInstance)
{
    m_PayloadSizeBytes = m_Config.BlockSizeSamplesPerChannel * (m_Config.Channels.size()) * (m_Config.GetBytesPerSample());
    m_DroppedBlockPayload.resize(m_PayloadSizeBytes);
}


std::uint64_t DataProducer::CalculateBlockTimeOffsetNs(std::uint64_t BlockIndex) const
{
    const auto ElapsedSamplesPerChannel =
        static_cast<long double>(BlockIndex) *
        static_cast<long double>(m_Config.BlockSizeSamplesPerChannel);

    const long double BlockTimeOffsetNs =
        (ElapsedSamplesPerChannel * m_NanosecondsPerSecond) /
        static_cast<long double>(m_Config.SampleRateHz);

    const long double RoundedBlockTimeOffsetNs = std::round(BlockTimeOffsetNs);

    return static_cast<std::uint64_t>(RoundedBlockTimeOffsetNs);
}


void DataProducer::UpdateHeadersForBlock(std::uint64_t BlockTimestampNs,
                                         std::uint64_t NextWriteSequence,
                                         PerSlotHeader& SlotHeader,
                                         GlobalControlHeader& GlobalHeader) const
{
    SlotHeader.TimestampNs = BlockTimestampNs;
    SlotHeader.SampleCountPerChannel = m_Config.BlockSizeSamplesPerChannel;
    SlotHeader.PayloadSizeBytes = m_PayloadSizeBytes;
    SlotHeader.BlockSequence.store(NextWriteSequence, std::memory_order_release);
    GlobalHeader.WriteSeq.store(NextWriteSequence, std::memory_order_release);
}


void DataProducer::ProduceBlock(std::uint64_t BlockIndex,
                                GlobalControlHeader& GlobalHeader)
{
    const std::uint64_t BlockTimestampNs = CalculateBlockTimeOffsetNs(BlockIndex);
    const std::uint64_t LastWrittenSequence =
        GlobalHeader.WriteSeq.load(std::memory_order_relaxed);
    const std::uint64_t LastReadSequence =
        GlobalHeader.ReadSeq.load(std::memory_order_acquire);

    if ((LastWrittenSequence - LastReadSequence) >= m_Config.SlotCount)
    {
        m_BlockAssembler.AssembleBlock(BlockTimestampNs, m_DroppedBlockPayload.data());
        GlobalHeader.DroppedBlocks.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    const std::uint64_t NextWriteSequence = LastWrittenSequence + 1;
    const auto SlotIndex =
        static_cast<std::size_t>((NextWriteSequence - 1) % m_Config.SlotCount);
    PerSlotHeader& SlotHeader = m_MemoryHandler.GetSlotHeader(SlotIndex);

    m_BlockAssembler.AssembleBlock(BlockTimestampNs,
                                   m_MemoryHandler.GetSlotPayloadStartingAddress(SlotIndex));

    UpdateHeadersForBlock(BlockTimestampNs, NextWriteSequence, SlotHeader, GlobalHeader);

    m_OsAbstractions.NotifyConsumer();
}


void DataProducer::Run()
{
    if (m_IsProducerRunning)
    {
        throw std::logic_error("data producer can only be run once");
    }

    m_IsProducerRunning = true;
    m_OsAbstractions.RegisterSignalHandlers();

    const std::uint64_t RunStartTimeNs = m_OsAbstractions.GetMonotonicTimeNs();
    std::uint64_t BlockIndex = 0;
    GlobalControlHeader& GlobalHeader = m_MemoryHandler.GetGlobalControlHeader();

    while (!m_OsAbstractions.IsTerminationRequested())
    {
        const std::uint64_t DeadlineOffsetNs = CalculateBlockTimeOffsetNs(BlockIndex + 1);
        m_OsAbstractions.WaitUntilTimeNs(RunStartTimeNs + DeadlineOffsetNs);
        ProduceBlock(BlockIndex, GlobalHeader);
        ++BlockIndex;
    }
}
