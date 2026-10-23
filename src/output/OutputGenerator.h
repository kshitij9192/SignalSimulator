#pragma once

#include "config/Config.h"
#include "disturbance/IDisturbanceGenerator.h"
#include "signal/ISignalGenerator.h"

#include <cstdint>
#include <memory>
#include <vector>

class OutputGenerator final
{
  public:
    OutputGenerator(
        const SimulatorConfig& Config, ISignalGenerator& SignalGenerator,
        const std::vector<std::unique_ptr<IDisturbanceGenerator>>& DisturbanceGenerators);
    ~OutputGenerator() = default;

    OutputGenerator(const OutputGenerator&) = delete;
    OutputGenerator& operator=(const OutputGenerator&) = delete;

    void GenerateCompositeSample(std::uint64_t SimulationTimeNs,
                                 std::vector<double>& OutChannelSamples);

  private:
    void ResetCurrentDisturbance();
    void ApplyAllDisturbances(std::uint64_t SimulationTimeNs);

    const SimulatorConfig& m_Config;
    ISignalGenerator& m_SignalGenerator;
    const std::vector<std::unique_ptr<IDisturbanceGenerator>>& m_DisturbanceGenerators;
    SampleDisturbance m_CurrentDisturbance;
    std::vector<double> m_BaseChannelSamples;
};
