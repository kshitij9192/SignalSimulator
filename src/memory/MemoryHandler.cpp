#include "memory/MemoryHandler.h"

#include <new>

MemoryHandler::MemoryHandler(const SimulatorConfig& rConfig, IOsAbstractions& rOsAbstractions)
    : m_rConfig(rConfig), m_rOsAbstractions(rOsAbstractions)
{
    Init();
}


MemoryHandler::~MemoryHandler()
{
}


std::size_t MemoryHandler::CalculatePayloadSize() const
{
    return m_rConfig.BlockSizeSamplesPerChannel * m_rConfig.Channels.size() *
           m_rConfig.GetBytesPerSample();
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
    return sizeof(GlobalControlHeader) + (m_rConfig.SlotCount * CalculateSlotSize());
}


void MemoryHandler::InitializeGlobalHeader()
{
    auto* pGlobalHeader = new (m_pSharedMemory) GlobalControlHeader{};
    pGlobalHeader->SlotCount = static_cast<std::uint32_t>(m_rConfig.SlotCount);
    pGlobalHeader->BlockSizeSamplesPerChannel = m_rConfig.BlockSizeSamplesPerChannel;
    pGlobalHeader->ChannelCount = m_rConfig.Channels.size();
    pGlobalHeader->SampleRateHz = m_rConfig.SampleRateHz;
    pGlobalHeader->Representation = m_rConfig.Representation;
    pGlobalHeader->Layout = m_rConfig.Layout;

    if (m_rConfig.Adc.has_value())
    {
        pGlobalHeader->Polarity = m_rConfig.Adc->Polarity;
        pGlobalHeader->AdcResolutionBits = m_rConfig.Adc->ResolutionBits;
        pGlobalHeader->AdcReferenceAmplitude = m_rConfig.Adc->ReferenceAmplitude;
    }

    pGlobalHeader->Magic = SharedMemoryMagic;
}


void MemoryHandler::InitializeSlotHeaders()
{
    auto* pBaseBytes = static_cast<std::byte*>(m_pSharedMemory);
    const std::size_t PayloadSize = CalculatePayloadSize();
    const std::size_t SlotSize = CalculateSlotSize();

    for (std::size_t SlotIndex = 0; SlotIndex < m_rConfig.SlotCount; ++SlotIndex)
    {
        const std::size_t SlotOffset = sizeof(GlobalControlHeader) + (SlotIndex * SlotSize);
        auto* pSlotHeader = new (pBaseBytes + SlotOffset) PerSlotHeader{};
        pSlotHeader->SampleCountPerChannel = m_rConfig.BlockSizeSamplesPerChannel;
        pSlotHeader->PayloadSizeBytes = PayloadSize;
    }
}


void MemoryHandler::Init()
{
    m_pSharedMemory = m_rOsAbstractions.CreateAndMapMemory(m_rConfig.SharedMemoryName,
                                                           CalculateTotalSharedMemorySize());
    InitializeSlotHeaders();
    InitializeGlobalHeader();
}


void* MemoryHandler::GetSharedMemoryBase()
{
    return m_pSharedMemory;
}
