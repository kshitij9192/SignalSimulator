#include "BlockAssembler.h"

#include <cmath>

namespace
{
constexpr double NanosecondsPerSecond = 1e9;
} // namespace


BlockAssembler::BlockAssembler(const SimulatorConfig& rConfig, OutputGenerator& rOutputGenerator,
                               const ISampleQuantizer& rSampleQuantizer)
    : m_rConfig(rConfig),
      m_rOutputGenerator(rOutputGenerator),
      m_rSampleQuantizer(rSampleQuantizer),
      m_ChannelSamples(rConfig.Channels.size(), 0.0)
{
}


std::uint64_t BlockAssembler::CalculateSampleTimestampNs(std::uint64_t BlockStartTimeNs,
                                                         std::size_t SampleIndex) const
{
    const double OffsetNs =
        (static_cast<double>(SampleIndex) * NanosecondsPerSecond) / m_rConfig.SampleRateHz;
    return BlockStartTimeNs + static_cast<std::uint64_t>(std::llround(OffsetNs));
}


std::size_t BlockAssembler::CalculateSampleByteOffset(std::size_t SampleIndex,
                                                      std::size_t ChannelIndex) const
{
    const std::size_t ChannelCount = m_rConfig.Channels.size();
    std::size_t ElementIndex = 0;

    if (m_rConfig.Layout == ChannelDataLayout::Interleaved)
    {
        ElementIndex = (SampleIndex * ChannelCount) + ChannelIndex;
    }
    else
    {
        ElementIndex = (ChannelIndex * m_rConfig.BlockSizeSamplesPerChannel) + SampleIndex;
    }

    return ElementIndex * m_rSampleQuantizer.GetBytesPerSample();
}


void BlockAssembler::AssembleBlock(std::uint64_t BlockStartTimeNs, std::byte* pPayloadDestination)
{
    const std::size_t ChannelCount = m_rConfig.Channels.size();

    for (std::size_t SampleIndex = 0; SampleIndex < m_rConfig.BlockSizeSamplesPerChannel;
         ++SampleIndex)
    {
        const std::uint64_t SampleTimeNs =
            CalculateSampleTimestampNs(BlockStartTimeNs, SampleIndex);
        m_rOutputGenerator.GenerateCompositeSample(SampleTimeNs, m_ChannelSamples);

        for (std::size_t ChannelIndex = 0; ChannelIndex < ChannelCount; ++ChannelIndex)
        {
            const std::size_t ByteOffset = CalculateSampleByteOffset(SampleIndex, ChannelIndex);
            m_rSampleQuantizer.QuantizeSampleAndCopyToMemory(m_ChannelSamples[ChannelIndex],
                                                             pPayloadDestination + ByteOffset);
        }
    }
}
