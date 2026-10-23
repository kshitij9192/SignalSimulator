#pragma once

#include "config/Config.h"
#include "platform/IOsAbstractions.h"

#include <atomic>
#include <cstddef>
#include <cstdint>

struct alignas(8) GlobalControlHeader
{
    static constexpr std::uint32_t m_SharedMemoryMagic = 0x53494753U;

    std::uint32_t Magic = 0;
    std::uint32_t SlotCount = 0;
    std::uint64_t BlockSizeSamplesPerChannel = 0;
    std::uint64_t ChannelCount = 0;
    double SampleRateHz = 0.0;
    SampleRepresentation Representation = SampleRepresentation::Float32;
    ChannelDataLayout Layout = ChannelDataLayout::Interleaved;
    AdcPolarity Polarity = AdcPolarity::Bipolar;
    std::uint32_t AdcResolutionBits = 0;
    double AdcReferenceAmplitude = 0.0;
    std::atomic<std::uint64_t> WriteSeq{0};
    std::atomic<std::uint64_t> ReadSeq{0};
    std::atomic<std::uint64_t> DroppedBlocks{0};
};


struct alignas(8) PerSlotHeader
{
    std::atomic<std::uint64_t> BlockSequence{0};
    std::uint64_t TimestampNs = 0;
    std::uint64_t SampleCountPerChannel = 0;
    std::uint64_t PayloadSizeBytes = 0;
};


class MemoryHandler final
{
  public:
    MemoryHandler(const SimulatorConfig& Config, IOsAbstractions& OsAbstractions);
    ~MemoryHandler();

    MemoryHandler(const MemoryHandler&) = delete;
    MemoryHandler& operator=(const MemoryHandler&) = delete;

    void* GetSharedMemoryBase();
    GlobalControlHeader& GetGlobalControlHeader();
    PerSlotHeader& GetSlotHeader(std::size_t SlotIndex);
    std::byte* GetSlotPayloadStartingAddress(std::size_t SlotIndex);

  private:
    std::size_t CalculatePayloadSize() const;
    std::size_t CalculateAlignedPayloadSize() const;
    std::size_t CalculateSlotSize() const;
    std::size_t CalculateTotalSharedMemorySize() const;
    void InitializeGlobalHeader();
    void InitializeSlotHeaders();
    void Init();

    const SimulatorConfig& m_Config;
    IOsAbstractions& m_OsAbstractions;
    void* m_SharedMemory = nullptr;
};
