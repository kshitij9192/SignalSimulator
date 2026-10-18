#pragma once

#include "config/Config.h"
#include "platform/IOsAbstractions.h"

#include <atomic>
#include <cstddef>
#include <cstdint>

inline constexpr std::uint32_t SharedMemoryMagic = 0x53494753U;


struct alignas(8) GlobalControlHeader
{
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
    MemoryHandler(const SimulatorConfig& rConfig, IOsAbstractions& rOsAbstractions);
    ~MemoryHandler();

    MemoryHandler(const MemoryHandler&) = delete;
    MemoryHandler& operator=(const MemoryHandler&) = delete;

    void* GetSharedMemoryBase();

  private:
    std::size_t CalculatePayloadSize() const;
    std::size_t CalculateAlignedPayloadSize() const;
    std::size_t CalculateSlotSize() const;
    std::size_t CalculateTotalSharedMemorySize() const;
    void InitializeGlobalHeader();
    void InitializeSlotHeaders();
    void Init();

    const SimulatorConfig& m_rConfig;
    IOsAbstractions& m_rOsAbstractions;
    void* m_pSharedMemory = nullptr;
};
