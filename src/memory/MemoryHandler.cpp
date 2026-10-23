#include "memory/MemoryHandler.h"

#include <new>
#include <stdexcept>

MemoryHandler::MemoryHandler(const SimulatorConfig& Config, IOsAbstractions& OsAbstractions)
    : m_Config(Config), m_OsAbstractions(OsAbstractions)
{
    Init();
}


MemoryHandler::~MemoryHandler()
{
}


std::size_t MemoryHandler::CalculatePayloadSize() const
{
    return m_Config.BlockSizeSamplesPerChannel * m_Config.Channels.size() *
           m_Config.GetBytesPerSample();
}


std::size_t MemoryHandler::CalculateAlignedPayloadSize() const
{
    constexpr std::size_t Alignment = alignof(PerSlotHeader);
    const std::size_t PayloadSize = CalculatePayloadSize();
    const std::size_t Remainder = PayloadSize % Alignment;
    std::size_t AlignedPayloadSize = PayloadSize;

    if (Remainder != 0)
    {
        AlignedPayloadSize = PayloadSize + (Alignment - Remainder);
    }

    return AlignedPayloadSize;
}


std::size_t MemoryHandler::CalculateSlotSize() const
{
    return sizeof(PerSlotHeader) + CalculateAlignedPayloadSize();
}


std::size_t MemoryHandler::CalculateTotalSharedMemorySize() const
{
    return sizeof(GlobalControlHeader) + (m_Config.SlotCount * CalculateSlotSize());
}


void MemoryHandler::InitializeGlobalHeader()
{
    auto* GlobalHeader = new (m_SharedMemory) GlobalControlHeader{};
    GlobalHeader->SlotCount = static_cast<std::uint32_t>(m_Config.SlotCount);
    GlobalHeader->BlockSizeSamplesPerChannel = m_Config.BlockSizeSamplesPerChannel;
    GlobalHeader->ChannelCount = m_Config.Channels.size();
    GlobalHeader->SampleRateHz = m_Config.SampleRateHz;
    GlobalHeader->Representation = m_Config.Representation;
    GlobalHeader->Layout = m_Config.Layout;

    if (m_Config.Adc.has_value())
    {
        GlobalHeader->Polarity = m_Config.Adc->Polarity;
        GlobalHeader->AdcResolutionBits = m_Config.Adc->ResolutionBits;
        GlobalHeader->AdcReferenceAmplitude = m_Config.Adc->ReferenceAmplitude;
    }

    GlobalHeader->Magic = GlobalControlHeader::m_SharedMemoryMagic;
}


void MemoryHandler::InitializeSlotHeaders()
{
    auto* BaseAddress = static_cast<std::byte*>(m_SharedMemory);
    const std::size_t PayloadSize = CalculatePayloadSize();
    const std::size_t SlotSize = CalculateSlotSize();

    for (std::size_t SlotIndex = 0; SlotIndex < m_Config.SlotCount; ++SlotIndex)
    {
        const std::size_t SlotOffset = sizeof(GlobalControlHeader) + (SlotIndex * SlotSize);
        auto* SlotHeader = new (BaseAddress + SlotOffset) PerSlotHeader{};
        SlotHeader->SampleCountPerChannel = m_Config.BlockSizeSamplesPerChannel;
        SlotHeader->PayloadSizeBytes = PayloadSize;
    }
}


void MemoryHandler::Init()
{
    m_SharedMemory = m_OsAbstractions.CreateAndMapMemory(m_Config.SharedMemoryName,
                                                         CalculateTotalSharedMemorySize());
    m_OsAbstractions.CreateNotificationSemaphore(m_Config.SemaphoreName);
    InitializeSlotHeaders();
    InitializeGlobalHeader();
}


void* MemoryHandler::GetSharedMemoryBase()
{
    return m_SharedMemory;
}


GlobalControlHeader& MemoryHandler::GetGlobalControlHeader()
{
    return *static_cast<GlobalControlHeader*>(m_SharedMemory);
}


PerSlotHeader& MemoryHandler::GetSlotHeader(std::size_t SlotIndex)
{
    auto* BaseAddress = static_cast<std::byte*>(m_SharedMemory);
    const std::size_t SlotOffset = sizeof(GlobalControlHeader) + (SlotIndex * CalculateSlotSize());
    return *reinterpret_cast<PerSlotHeader*>(BaseAddress + SlotOffset);
}


std::byte* MemoryHandler::GetSlotPayloadStartingAddress(std::size_t SlotIndex)
{
    auto& SlotHeader = GetSlotHeader(SlotIndex);
    return reinterpret_cast<std::byte*>(&SlotHeader) + sizeof(PerSlotHeader);
}
