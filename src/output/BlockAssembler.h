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
    BlockAssembler(const SimulatorConfig& rConfig, OutputGenerator& rOutputGenerator,
                   const ISampleQuantizer& rSampleQuantizer);
    ~BlockAssembler() = default;

    BlockAssembler(const BlockAssembler&) = delete;
    BlockAssembler& operator=(const BlockAssembler&) = delete;

    void AssembleBlock(std::uint64_t BlockStartTimeNs, std::byte* pPayloadDestination);

  private:
    std::uint64_t CalculateSampleTimestampNs(std::uint64_t BlockStartTimeNs,
                                             std::size_t SampleIndex) const;
    std::size_t CalculateSampleByteOffset(std::size_t SampleIndex,
                                          std::size_t ChannelIndex) const;

    const SimulatorConfig& m_rConfig;
    OutputGenerator& m_rOutputGenerator;
    const ISampleQuantizer& m_rSampleQuantizer;
    std::vector<double> m_ChannelSamples;
};
