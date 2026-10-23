#pragma once

#include <cstdint>
#include <vector>

struct ChannelDisturbance
{
    double AmplitudeScale = 1.0;
    double AdditiveOffset = 0.0;
};


struct SampleDisturbance
{
    double DeltaFrequencyHz = 0.0;
    std::vector<ChannelDisturbance> Channels;
};


class IDisturbanceGenerator
{
  public:
    virtual ~IDisturbanceGenerator() = default;

    virtual void ApplyDisturbance(std::uint64_t SimulationTimeNs,
                                  SampleDisturbance& OutDisturbance) = 0;
};
