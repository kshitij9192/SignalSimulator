#include "output/BlockAssembler.h"

#include <cmath>

BlockAssembler::BlockAssembler(const SimulatorConfig& Config, OutputGenerator& OutputGeneratorInstance,
                               const ISampleQuantizer& SampleQuantizer)
    : m_Config(Config),
      m_OutputGenerator(OutputGeneratorInstance),
      m_SampleQuantizer(SampleQuantizer),
      m_ChannelSamples(Config.Channels.size(), 0.0)
{
}


std::uint64_t BlockAssembler::CalculateSampleTimestampNs(std::uint64_t BlockStartTimeNs,
                                                         std::size_t SampleIndex) const
{
    const double OffsetNs =
        (static_cast<double>(SampleIndex) * m_NanosecondsPerSecond) / m_Config.SampleRateHz;
    return BlockStartTimeNs + static_cast<std::uint64_t>(std::llround(OffsetNs));
}


std::size_t BlockAssembler::CalculateSampleByteOffset(std::size_t SampleIndex,
                                                      std::size_t ChannelIndex) const
{
    const std::size_t ChannelCount = m_Config.Channels.size();
    std::size_t ElementIndex = 0;

    if (m_Config.Layout == ChannelDataLayout::Interleaved)
    {
        ElementIndex = (SampleIndex * ChannelCount) + ChannelIndex;
    }
    else
    {
        ElementIndex = (ChannelIndex * m_Config.BlockSizeSamplesPerChannel) + SampleIndex;
    }

    return ElementIndex * m_SampleQuantizer.GetBytesPerSample();
}


void BlockAssembler::AssembleBlock(std::uint64_t BlockStartTimeNs, std::byte* PayloadDestination)
{
    const std::size_t ChannelCount = m_Config.Channels.size();

    for (std::size_t SampleIndex = 0; SampleIndex < m_Config.BlockSizeSamplesPerChannel;
         ++SampleIndex)
    {
        const std::uint64_t SampleTimeNs =
            CalculateSampleTimestampNs(BlockStartTimeNs, SampleIndex);
        m_OutputGenerator.GenerateCompositeSample(SampleTimeNs, m_ChannelSamples);

        for (std::size_t ChannelIndex = 0; ChannelIndex < ChannelCount; ++ChannelIndex)
        {
            const std::size_t ByteOffset = CalculateSampleByteOffset(SampleIndex, ChannelIndex);
            m_SampleQuantizer.QuantizeSampleAndCopyToMemory(m_ChannelSamples[ChannelIndex],
                                                            PayloadDestination + ByteOffset);
        }
    }
}
