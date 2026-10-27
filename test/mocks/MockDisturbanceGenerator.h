#pragma once

#include "disturbance/IDisturbanceGenerator.h"

#include <cstddef>
#include <cstdint>

class MockDisturbanceGenerator final : public IDisturbanceGenerator
{
  public:
    MockDisturbanceGenerator(std::uint64_t StartTimeNs, std::uint64_t EndTimeNs,
                             std::size_t TargetChannelIndex, double AmplitudeScaleMultiplier,
                             double AdditiveOffsetDelta, double FrequencyDeltaHz)
        : m_StartTimeNs(StartTimeNs),
          m_EndTimeNs(EndTimeNs),
          m_TargetChannelIndex(TargetChannelIndex),
          m_AmplitudeScaleMultiplier(AmplitudeScaleMultiplier),
          m_AdditiveOffsetDelta(AdditiveOffsetDelta),
          m_FrequencyDeltaHz(FrequencyDeltaHz)
    {
    }


    void ApplyDisturbance(std::uint64_t SimulationTimeNs,
                          SampleDisturbance& OutDisturbance) override
    {
        if (SimulationTimeNs < m_StartTimeNs || SimulationTimeNs >= m_EndTimeNs)
        {
            return;
        }

        OutDisturbance.DeltaFrequencyHz += m_FrequencyDeltaHz;

        if (m_TargetChannelIndex < OutDisturbance.Channels.size())
        {
            ChannelDisturbance& Channel = OutDisturbance.Channels[m_TargetChannelIndex];
            Channel.AmplitudeScale *= m_AmplitudeScaleMultiplier;
            Channel.AdditiveOffset += m_AdditiveOffsetDelta;
        }
    }

  private:
    std::uint64_t m_StartTimeNs = 0;
    std::uint64_t m_EndTimeNs = 0;
    std::size_t m_TargetChannelIndex = 0;
    double m_AmplitudeScaleMultiplier = 1.0;
    double m_AdditiveOffsetDelta = 0.0;
    double m_FrequencyDeltaHz = 0.0;
};
