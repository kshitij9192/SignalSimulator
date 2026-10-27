#pragma once

#include "signal/ISignalGenerator.h"

#include <cstddef>
#include <utility>
#include <vector>

class MockSignalGenerator final : public ISignalGenerator
{
  public:
    explicit MockSignalGenerator(std::vector<double> BaseChannelSamples)
        : m_ConfiguredBaseSamples(std::move(BaseChannelSamples))
    {
    }


    void GenerateSample(double DeltaFrequencyHz, std::vector<double>& OutChannelSamples) override
    {
        m_LastReceivedDeltaFrequencyHz = DeltaFrequencyHz;
        ++m_GenerateSampleCallCount;
        OutChannelSamples = m_ConfiguredBaseSamples;
    }


    double GetCurrentPhaseRadians() const override
    {
        return 0.0;
    }


    double GetLastReceivedDeltaFrequencyHz() const
    {
        return m_LastReceivedDeltaFrequencyHz;
    }


    std::size_t GetGenerateSampleCallCount() const
    {
        return m_GenerateSampleCallCount;
    }

  private:
    std::vector<double> m_ConfiguredBaseSamples;
    double m_LastReceivedDeltaFrequencyHz = 0.0;
    std::size_t m_GenerateSampleCallCount = 0;
};
