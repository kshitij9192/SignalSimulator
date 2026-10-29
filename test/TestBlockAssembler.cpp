#include "config/Config.h"
#include "disturbance/IDisturbanceGenerator.h"
#include "mocks/MockDisturbanceGenerator.h"
#include "mocks/MockSignalGenerator.h"
#include "output/AdcSampleQuantizer.h"
#include "output/BlockAssembler.h"
#include "output/Float32SampleQuantizer.h"
#include "output/OutputGenerator.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

namespace
{
class SequentialMockSignalGenerator final : public ISignalGenerator
{
  public:
    explicit SequentialMockSignalGenerator(std::size_t ChannelCount)
        : m_ChannelCount(ChannelCount)
    {
    }


    void GenerateSample(double, std::vector<double>& OutChannelSamples) override
    {
        OutChannelSamples.resize(m_ChannelCount);
        for (std::size_t ChannelIndex = 0; ChannelIndex < m_ChannelCount; ++ChannelIndex)
        {
            OutChannelSamples[ChannelIndex] =
                static_cast<double>((m_SampleCallIndex * 10U) + ChannelIndex + 1U);
        }
        ++m_SampleCallIndex;
    }


    double GetCurrentPhaseRadians() const override
    {
        return 0.0;
    }

  private:
    std::size_t m_ChannelCount = 0;
    std::size_t m_SampleCallIndex = 0;
};


SimulatorConfig CreateBlockAssemblerConfig(ChannelDataLayout Layout,
                                           std::size_t BlockSizeSamplesPerChannel,
                                           double SampleRateHz)
{
    SimulatorConfig Config{};
    Config.SampleRateHz = SampleRateHz;
    Config.BlockSizeSamplesPerChannel = BlockSizeSamplesPerChannel;
    Config.Layout = Layout;
    Config.Representation = SampleRepresentation::Float32;
    Config.Channels = {
        ChannelConfig{0, 0.0},
        ChannelConfig{1, 120.0},
        ChannelConfig{2, 240.0},
    };
    return Config;
}
} // namespace


TEST_CASE("BlockAssembler arranges multi-channel samples according to configured layout",
          "[BlockAssembler]")
{
    using Catch::Matchers::WithinAbs;

    constexpr std::size_t SamplesPerChannel = 4U;
    constexpr std::size_t ChannelCount = 3U;
    constexpr std::size_t TotalSamples = SamplesPerChannel * ChannelCount;

    SECTION("Interleaved layout writes samples in sample-major order [S0C0, S0C1, S0C2, S1C0, ...]")
    {
        const SimulatorConfig Config = CreateBlockAssemblerConfig(
            ChannelDataLayout::Interleaved, SamplesPerChannel, 1'000.0);
        SequentialMockSignalGenerator SignalGenerator(ChannelCount);
        const std::vector<std::unique_ptr<IDisturbanceGenerator>> DisturbanceGenerators{};
        OutputGenerator Output(Config, SignalGenerator, DisturbanceGenerators);
        Float32SampleQuantizer Quantizer;
        BlockAssembler Assembler(Config, Output, Quantizer);

        std::vector<std::byte> PayloadBuffer(TotalSamples * sizeof(float));
        Assembler.AssembleBlock(0U, PayloadBuffer.data());

        std::vector<float> UnpackedSamples(TotalSamples, 0.0F);
        std::memcpy(UnpackedSamples.data(), PayloadBuffer.data(), PayloadBuffer.size());

        const std::vector<float> ExpectedInterleaved = {
            1.0F, 2.0F, 3.0F,    // Sample 0: C0, C1, C2
            11.0F, 12.0F, 13.0F, // Sample 1: C0, C1, C2
            21.0F, 22.0F, 23.0F, // Sample 2: C0, C1, C2
            31.0F, 32.0F, 33.0F, // Sample 3: C0, C1, C2
        };

        for (std::size_t Index = 0; Index < TotalSamples; ++Index)
        {
            CHECK_THAT(UnpackedSamples[Index], WithinAbs(ExpectedInterleaved[Index], 1e-6F));
        }
    }

    SECTION("Planar layout writes samples in channel-major order [S0C0..S3C0, S0C1..S3C1, ...]")
    {
        const SimulatorConfig Config = CreateBlockAssemblerConfig(
            ChannelDataLayout::Planar, SamplesPerChannel, 1'000.0);
        SequentialMockSignalGenerator SignalGenerator(ChannelCount);
        const std::vector<std::unique_ptr<IDisturbanceGenerator>> DisturbanceGenerators{};
        OutputGenerator Output(Config, SignalGenerator, DisturbanceGenerators);
        Float32SampleQuantizer Quantizer;
        BlockAssembler Assembler(Config, Output, Quantizer);

        std::vector<std::byte> PayloadBuffer(TotalSamples * sizeof(float));
        Assembler.AssembleBlock(0U, PayloadBuffer.data());

        std::vector<float> UnpackedSamples(TotalSamples, 0.0F);
        std::memcpy(UnpackedSamples.data(), PayloadBuffer.data(), PayloadBuffer.size());

        const std::vector<float> ExpectedPlanar = {
            1.0F, 11.0F, 21.0F, 31.0F, // Channel 0: S0, S1, S2, S3
            2.0F, 12.0F, 22.0F, 32.0F, // Channel 1: S0, S1, S2, S3
            3.0F, 13.0F, 23.0F, 33.0F, // Channel 2: S0, S1, S2, S3
        };

        for (std::size_t Index = 0; Index < TotalSamples; ++Index)
        {
            CHECK_THAT(UnpackedSamples[Index], WithinAbs(ExpectedPlanar[Index], 1e-6F));
        }
    }
}


TEST_CASE("BlockAssembler advances intra-block sample timestamps from BlockStartTimeNs",
          "[BlockAssembler]")
{
    using Catch::Matchers::WithinAbs;

    SimulatorConfig Config{};
    Config.SampleRateHz = 3'000.0; // Offsets: 0 ns, 333'333 ns, 666'667 ns, 1'000'000 ns
    Config.BlockSizeSamplesPerChannel = 4U;
    Config.Layout = ChannelDataLayout::Interleaved;
    Config.Representation = SampleRepresentation::Float32;
    Config.Channels = {ChannelConfig{0, 0.0}};

    MockSignalGenerator SignalGenerator({10.0});
    std::vector<std::unique_ptr<IDisturbanceGenerator>> DisturbanceGenerators;
    // Active window [1'333'333, 1'666'668) -> hits SampleIndex 1 (1'333'333) and SampleIndex 2 (1'666'667)
    // when BlockStartTimeNs = 1'000'000
    DisturbanceGenerators.push_back(
        std::make_unique<MockDisturbanceGenerator>(1'333'333ULL, 1'666'668ULL, 0U, 2.0, 5.0, 0.0));

    OutputGenerator Output(Config, SignalGenerator, DisturbanceGenerators);
    Float32SampleQuantizer Quantizer;
    BlockAssembler Assembler(Config, Output, Quantizer);

    std::vector<std::byte> PayloadBuffer(4U * sizeof(float));
    Assembler.AssembleBlock(1'000'000ULL, PayloadBuffer.data());

    std::vector<float> UnpackedSamples(4U, 0.0F);
    std::memcpy(UnpackedSamples.data(), PayloadBuffer.data(), PayloadBuffer.size());

    // Sample 0 at 1'000'000 ns: outside window -> 10.0
    CHECK_THAT(UnpackedSamples[0], WithinAbs(10.0F, 1e-6F));
    // Sample 1 at 1'333'333 ns: inside window -> (2.0 * 10.0) + 5.0 = 25.0
    CHECK_THAT(UnpackedSamples[1], WithinAbs(25.0F, 1e-6F));
    // Sample 2 at 1'666'667 ns: inside window -> (2.0 * 10.0) + 5.0 = 25.0
    CHECK_THAT(UnpackedSamples[2], WithinAbs(25.0F, 1e-6F));
    // Sample 3 at 2'000'000 ns: outside window -> 10.0
    CHECK_THAT(UnpackedSamples[3], WithinAbs(10.0F, 1e-6F));
}


TEST_CASE("BlockAssembler packs quantized ADC samples using the configured byte stride",
          "[BlockAssembler]")
{
    SECTION("8-bit unipolar ADC packs 1-byte samples contiguously without overwriting sentinel")
    {
        SimulatorConfig Config{};
        Config.SampleRateHz = 1'000.0;
        Config.BlockSizeSamplesPerChannel = 2U;
        Config.Layout = ChannelDataLayout::Interleaved;
        Config.Representation = SampleRepresentation::AdcInteger;
        Config.Adc = AdcConfig{10.0, AdcPolarity::Unipolar, 8};
        Config.Channels = {ChannelConfig{0, 0.0}, ChannelConfig{1, 120.0}};

        MockSignalGenerator SignalGenerator({0.0, 10.0});
        const std::vector<std::unique_ptr<IDisturbanceGenerator>> DisturbanceGenerators{};
        OutputGenerator Output(Config, SignalGenerator, DisturbanceGenerators);
        AdcSampleQuantizer Quantizer(*Config.Adc);
        BlockAssembler Assembler(Config, Output, Quantizer);

        constexpr std::byte Sentinel{0x5A};
        std::vector<std::byte> PayloadBuffer(5U, Sentinel);
        Assembler.AssembleBlock(0U, PayloadBuffer.data());

        CHECK(PayloadBuffer[0] == std::byte{0x00});
        CHECK(PayloadBuffer[1] == std::byte{0xFF});
        CHECK(PayloadBuffer[2] == std::byte{0x00});
        CHECK(PayloadBuffer[3] == std::byte{0xFF});
        CHECK(PayloadBuffer[4] == Sentinel);
    }

    SECTION("16-bit bipolar ADC packs 2-byte samples in planar layout")
    {
        SimulatorConfig Config{};
        Config.SampleRateHz = 1'000.0;
        Config.BlockSizeSamplesPerChannel = 2U;
        Config.Layout = ChannelDataLayout::Planar;
        Config.Representation = SampleRepresentation::AdcInteger;
        Config.Adc = AdcConfig{10.0, AdcPolarity::Bipolar, 16};
        Config.Channels = {ChannelConfig{0, 0.0}, ChannelConfig{1, 120.0}};

        MockSignalGenerator SignalGenerator({-10.0, 10.0});
        const std::vector<std::unique_ptr<IDisturbanceGenerator>> DisturbanceGenerators{};
        OutputGenerator Output(Config, SignalGenerator, DisturbanceGenerators);
        AdcSampleQuantizer Quantizer(*Config.Adc);
        BlockAssembler Assembler(Config, Output, Quantizer);

        std::vector<std::byte> PayloadBuffer(4U * sizeof(std::int16_t));
        Assembler.AssembleBlock(0U, PayloadBuffer.data());

        std::vector<std::int16_t> UnpackedSamples(4U, 0);
        std::memcpy(UnpackedSamples.data(), PayloadBuffer.data(), PayloadBuffer.size());

        // Planar order: [Ch0_S0, Ch0_S1, Ch1_S0, Ch1_S1]
        CHECK(UnpackedSamples[0] == -32768);
        CHECK(UnpackedSamples[1] == -32768);
        CHECK(UnpackedSamples[2] == 32767);
        CHECK(UnpackedSamples[3] == 32767);
    }
}
