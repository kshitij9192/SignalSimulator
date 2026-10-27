#include "config/Config.h"
#include "memory/MemoryHandler.h"
#include "mocks/MockOsAbstractions.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cstddef>
#include <cstdint>

class MemoryHandlerTestFixture
{
  protected:
    MemoryHandlerTestFixture()
    {
        m_Config.FrequencyHz = 50.0;
        m_Config.NominalAmplitude = 3.3;
        m_Config.Offset = 0.0;
        m_Config.SamplesPerCycle = 128;
        m_Config.SampleRateHz = 6400.0;
        m_Config.Channels = {
            ChannelConfig{0, 0.0},
            ChannelConfig{1, 120.0},
            ChannelConfig{2, 240.0},
        };
        m_Config.Representation = SampleRepresentation::Float32;
        m_Config.Layout = ChannelDataLayout::Interleaved;
        m_Config.SlotCount = 4;
        m_Config.BlockSizeSamplesPerChannel = 16;
        m_Config.SharedMemoryName = "/test_memory_handler_shm";
        m_Config.SemaphoreName = "/test_memory_handler_sem";
    }

    SimulatorConfig m_Config;
    MockOsAbstractions m_MockOs;
};


TEST_CASE_METHOD(MemoryHandlerTestFixture,
                 "MemoryHandler allocates expected size with alignment and initializes headers",
                 "[MemoryHandler]")
{
    using Catch::Matchers::WithinAbs;

    SECTION("Float32 configuration initializes GlobalControlHeader and aligned PerSlotHeaders")
    {
        MemoryHandler Handler(m_Config, m_MockOs);

        const std::size_t ExpectedPayloadSize = 16U * 3U * sizeof(float);
        const std::size_t ExpectedSlotSize = sizeof(PerSlotHeader) + ExpectedPayloadSize;
        const std::size_t ExpectedTotalSize =
            sizeof(GlobalControlHeader) + (m_Config.SlotCount * ExpectedSlotSize);

        CHECK(m_MockOs.GetSharedMemoryName() == m_Config.SharedMemoryName);
        CHECK(m_MockOs.GetSemaphoreName() == m_Config.SemaphoreName);
        CHECK(m_MockOs.IsSemaphoreCreated());
        CHECK(m_MockOs.GetAllocatedSizeBytes() == ExpectedTotalSize);

        const auto BaseAddressValue =
            reinterpret_cast<std::uintptr_t>(Handler.GetSharedMemoryBase());
        CHECK((BaseAddressValue % alignof(GlobalControlHeader)) == 0);

        GlobalControlHeader& GlobalHeader = Handler.GetGlobalControlHeader();
        CHECK(GlobalHeader.Magic.load(std::memory_order_acquire) ==
              GlobalControlHeader::m_SharedMemoryMagic);
        CHECK(GlobalHeader.SlotCount == m_Config.SlotCount);
        CHECK(GlobalHeader.BlockSizeSamplesPerChannel == m_Config.BlockSizeSamplesPerChannel);
        CHECK(GlobalHeader.ChannelCount == m_Config.Channels.size());
        CHECK_THAT(GlobalHeader.SampleRateHz, WithinAbs(m_Config.SampleRateHz, 1e-12));
        CHECK(GlobalHeader.Representation == SampleRepresentation::Float32);
        CHECK(GlobalHeader.Layout == ChannelDataLayout::Interleaved);
        CHECK(GlobalHeader.AdcResolutionBits == 0);
        CHECK_THAT(GlobalHeader.AdcReferenceAmplitude, WithinAbs(0.0, 1e-12));
        CHECK(GlobalHeader.WriteSeq.load() == 0);
        CHECK(GlobalHeader.ReadSeq.load() == 0);
        CHECK(GlobalHeader.DroppedBlocks.load() == 0);

        for (std::size_t SlotIndex = 0; SlotIndex < m_Config.SlotCount; ++SlotIndex)
        {
            PerSlotHeader& SlotHeader = Handler.GetSlotHeader(SlotIndex);
            const auto SlotAddressValue = reinterpret_cast<std::uintptr_t>(&SlotHeader);

            CHECK((SlotAddressValue % alignof(PerSlotHeader)) == 0);
            CHECK(SlotHeader.BlockSequence.load() == 0);
            CHECK(SlotHeader.TimestampNs == 0);
            CHECK(SlotHeader.SampleCountPerChannel == m_Config.BlockSizeSamplesPerChannel);
            CHECK(SlotHeader.PayloadSizeBytes == ExpectedPayloadSize);

            const std::byte* ExpectedPayloadAddress =
                reinterpret_cast<const std::byte*>(&SlotHeader) + sizeof(PerSlotHeader);
            CHECK(Handler.GetSlotPayloadStartingAddress(SlotIndex) == ExpectedPayloadAddress);
        }
    }

    SECTION("Unaligned ADC payload is padded so every PerSlotHeader stays 8-byte aligned")
    {
        m_Config.Representation = SampleRepresentation::AdcInteger;
        m_Config.Layout = ChannelDataLayout::Planar;
        m_Config.Adc = AdcConfig{5.0, AdcPolarity::Unipolar, 8};
        m_Config.Channels = {ChannelConfig{0, 0.0}};
        m_Config.BlockSizeSamplesPerChannel = 3;
        m_Config.SlotCount = 3;

        MemoryHandler Handler(m_Config, m_MockOs);

        const std::size_t ExpectedRawPayloadSize = 3U;
        const std::size_t ExpectedAlignedPayloadSize = 8U;
        const std::size_t ExpectedSlotSize =
            sizeof(PerSlotHeader) + ExpectedAlignedPayloadSize;
        const std::size_t ExpectedTotalSize =
            sizeof(GlobalControlHeader) + (m_Config.SlotCount * ExpectedSlotSize);

        CHECK(m_MockOs.GetAllocatedSizeBytes() == ExpectedTotalSize);

        GlobalControlHeader& GlobalHeader = Handler.GetGlobalControlHeader();
        CHECK(GlobalHeader.Representation == SampleRepresentation::AdcInteger);
        CHECK(GlobalHeader.Layout == ChannelDataLayout::Planar);
        CHECK(GlobalHeader.Polarity == AdcPolarity::Unipolar);
        CHECK(GlobalHeader.AdcResolutionBits == 8);
        CHECK_THAT(GlobalHeader.AdcReferenceAmplitude, WithinAbs(5.0, 1e-12));

        for (std::size_t SlotIndex = 0; SlotIndex < m_Config.SlotCount; ++SlotIndex)
        {
            PerSlotHeader& SlotHeader = Handler.GetSlotHeader(SlotIndex);
            const auto SlotAddressValue = reinterpret_cast<std::uintptr_t>(&SlotHeader);

            CHECK((SlotAddressValue % alignof(PerSlotHeader)) == 0);
            CHECK(SlotHeader.SampleCountPerChannel == m_Config.BlockSizeSamplesPerChannel);
            CHECK(SlotHeader.PayloadSizeBytes == ExpectedRawPayloadSize);
        }
    }
}


TEST_CASE_METHOD(MemoryHandlerTestFixture,
                 "MemoryHandler CalculatePayloadSize returns expected size in bytes",
                 "[MemoryHandler]")
{
    SECTION("Float32 representation uses 4 bytes per sample")
    {
        m_Config.Representation = SampleRepresentation::Float32;
        m_Config.Channels = {ChannelConfig{0, 0.0}, ChannelConfig{1, 120.0}};
        m_Config.BlockSizeSamplesPerChannel = 10;

        MemoryHandler Handler(m_Config, m_MockOs);

        CHECK(Handler.CalculatePayloadSize() == (10U * 2U * 4U));
    }

    SECTION("8-bit ADC representation uses 1 byte per sample")
    {
        m_Config.Representation = SampleRepresentation::AdcInteger;
        m_Config.Adc = AdcConfig{3.3, AdcPolarity::Unipolar, 8};
        m_Config.Channels = {
            ChannelConfig{0, 0.0},
            ChannelConfig{1, 120.0},
            ChannelConfig{2, 240.0},
        };
        m_Config.BlockSizeSamplesPerChannel = 5;

        MemoryHandler Handler(m_Config, m_MockOs);

        CHECK(Handler.CalculatePayloadSize() == (5U * 3U * 1U));
    }

    SECTION("16-bit ADC representation uses 2 bytes per sample")
    {
        m_Config.Representation = SampleRepresentation::AdcInteger;
        m_Config.Adc = AdcConfig{3.3, AdcPolarity::Bipolar, 16};
        m_Config.Channels = {ChannelConfig{0, 0.0}, ChannelConfig{1, 180.0}};
        m_Config.BlockSizeSamplesPerChannel = 32;

        MemoryHandler Handler(m_Config, m_MockOs);

        CHECK(Handler.CalculatePayloadSize() == (32U * 2U * 2U));
    }

    SECTION("24-bit ADC representation uses 4 bytes per sample")
    {
        m_Config.Representation = SampleRepresentation::AdcInteger;
        m_Config.Adc = AdcConfig{5.0, AdcPolarity::Bipolar, 24};
        m_Config.Channels = {ChannelConfig{0, 0.0}};
        m_Config.BlockSizeSamplesPerChannel = 64;

        MemoryHandler Handler(m_Config, m_MockOs);

        CHECK(Handler.CalculatePayloadSize() == (64U * 1U * 4U));
    }
}
