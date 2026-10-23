#pragma once

#include "config/Config.h"

#include <cstddef>
#include <cstdint>
#include <vector>

class BlockAssembler;
class IOsAbstractions;
class MemoryHandler;
struct GlobalControlHeader;
struct PerSlotHeader;

class DataProducer final
{
  public:
    DataProducer(const SimulatorConfig& Config, MemoryHandler& MemoryHandlerInstance,
                 IOsAbstractions& OsAbstractions, BlockAssembler& BlockAssemblerInstance);
    ~DataProducer() = default;

    DataProducer(const DataProducer&) = delete;
    DataProducer& operator=(const DataProducer&) = delete;

    void Run();

  private:
    std::uint64_t CalculateBlockTimeOffsetNs(std::uint64_t BlockIndex) const;
    void UpdateHeadersForBlock(std::uint64_t BlockTimestampNs,
                               std::uint64_t NextWriteSequence,
                               PerSlotHeader& SlotHeader,
                               GlobalControlHeader& GlobalHeader) const;
    void ProduceBlock(std::uint64_t BlockIndex, GlobalControlHeader& GlobalHeader);

    static constexpr long double m_NanosecondsPerSecond = 1'000'000'000.0L;

    const SimulatorConfig& m_Config;
    MemoryHandler& m_MemoryHandler;
    IOsAbstractions& m_OsAbstractions;
    BlockAssembler& m_BlockAssembler;
    std::vector<std::byte> m_DroppedBlockPayload;
    std::size_t m_PayloadSizeBytes = 0;
    bool m_IsProducerRunning = false;
};
