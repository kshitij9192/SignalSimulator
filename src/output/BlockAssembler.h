#pragma once

#include "config/Config.h"
#include "output/ISampleQuantizer.h"
#include "output/OutputGenerator.h"

#include <cstddef>
#include <cstdint>
#include <vector>

class BlockAssembler final
{
  public:
    BlockAssembler(const SimulatorConfig& Config, OutputGenerator& OutputGeneratorInstance,
                   const ISampleQuantizer& SampleQuantizer);
    ~BlockAssembler() = default;

    BlockAssembler(const BlockAssembler&) = delete;
    BlockAssembler& operator=(const BlockAssembler&) = delete;

    void AssembleBlock(std::uint64_t BlockStartTimeNs, std::byte* PayloadDestination);

  private:
    void InitializeSampleTimeOffsetsNs();
    std::size_t CalculateSampleByteOffset(std::size_t SampleIndex,
                                          std::size_t ChannelIndex) const;

    static constexpr double m_NanosecondsPerSecond = 1e9;

    const SimulatorConfig& m_Config;
    OutputGenerator& m_OutputGenerator;
    const ISampleQuantizer& m_SampleQuantizer;
    std::vector<double> m_ChannelSamples;
    std::vector<std::uint64_t> m_SampleTimeOffsetsNs;
};
