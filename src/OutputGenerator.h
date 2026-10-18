#pragma once

#include "Config.h"
#include "IDisturbanceGenerator.h"
#include "ISignalGenerator.h"

#include <cstdint>
#include <memory>
#include <vector>

class OutputGenerator final
{
  public:
    OutputGenerator(
        const SimulatorConfig& rConfig, ISignalGenerator& rSignalGenerator,
        const std::vector<std::unique_ptr<IDisturbanceGenerator>>& rDisturbanceGenerators);
    ~OutputGenerator() = default;

    OutputGenerator(const OutputGenerator&) = delete;
    OutputGenerator& operator=(const OutputGenerator&) = delete;

    void GenerateCompositeSample(std::uint64_t SimulationTimeNs,
                                 std::vector<double>& rOutChannelSamples);

  private:
    void ResetCurrentDisturbance();
    void EvaluateAllDisturbances(std::uint64_t SimulationTimeNs);

    const SimulatorConfig& m_rConfig;
    ISignalGenerator& m_rSignalGenerator;
    const std::vector<std::unique_ptr<IDisturbanceGenerator>>& m_rDisturbanceGenerators;
    SampleDisturbance m_CurrentDisturbance;
    std::vector<double> m_BaseChannelSamples;
};
