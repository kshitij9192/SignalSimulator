#include "MemoryHandler.h"
#include "ConfigParser.h"

#include <new>

MemoryHandler::MemoryHandler(const SimulatorConfig& rConfig) : m_rConfig(rConfig)
{
    Init();
}


MemoryHandler::~MemoryHandler()
{
}


std::size_t MemoryHandler::CalculatePayloadSize() const
{
    const std::size_t ChannelCount = m_rConfig.Channels.size();
    std::size_t BytesPerSample = 0;

    if (m_rConfig.Representation == SampleRepresentation::Float32)
    {
        BytesPerSample = sizeof(float);
    }
    else
    {
        switch (m_rConfig.Adc->ResolutionBits)
        {
        case 8:
            BytesPerSample = 1;
            break;
        case 12:
        case 16:
            BytesPerSample = 2;
            break;
        case 24:
            BytesPerSample = 4;
            break;
        }
    }

    return m_rConfig.BlockSizeSamplesPerChannel * ChannelCount * BytesPerSample;
}


std::size_t MemoryHandler::CalculateAlignedPayloadSize() const
{
    constexpr std::size_t Alignment = alignof(PerSlotHeader);
    const std::size_t PayloadSize = CalculatePayloadSize();
    const std::size_t Remainder = PayloadSize % Alignment;

    if (Remainder == 0)
    {
        return PayloadSize;
    }

    return PayloadSize + (Alignment - Remainder);
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
        pGlobalHeader->AdcReferenceVoltageV = m_rConfig.Adc->ReferenceVoltageV;
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
    m_pSharedMemory = m_OsAbstractions.CreateAndMapMemory(m_rConfig.SharedMemoryName,
                                                          CalculateTotalSharedMemorySize());
    InitializeSlotHeaders();
    InitializeGlobalHeader();
}


void* MemoryHandler::GetSharedMemoryBase()
{
    return m_pSharedMemory;
}
