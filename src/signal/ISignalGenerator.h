#pragma once

#include <vector>

class ISignalGenerator
{
  public:
    virtual ~ISignalGenerator() = default;

    virtual void GenerateSample(double DeltaFrequencyHz,
                                std::vector<double>& OutChannelSamples) = 0;
    virtual double GetCurrentPhaseRadians() const = 0;
};
