#include "disturbance/SurgeDisturbanceGenerator.h"

#include <cmath>

SurgeDisturbanceGenerator::SurgeDisturbanceGenerator(const SurgeEvent& Event,
                                                     std::size_t ChannelCount)
    : m_Event(Event), m_ChannelCount(ChannelCount)
{
}


bool SurgeDisturbanceGenerator::IsEventActive(std::uint64_t SimulationTimeNs) const
{
    return SimulationTimeNs >= m_Event.StartTimeNs &&
           (SimulationTimeNs - m_Event.StartTimeNs) < m_Event.DurationNs;
}


void SurgeDisturbanceGenerator::ApplyDisturbance(std::uint64_t SimulationTimeNs,
                                                 SampleDisturbance& OutDisturbance)
{
    if (OutDisturbance.Channels.size() != m_ChannelCount)
    {
        OutDisturbance.Channels.assign(m_ChannelCount, ChannelDisturbance{});
    }

    if (IsEventActive(SimulationTimeNs))
    {
        const double ElapsedNs = static_cast<double>(SimulationTimeNs - m_Event.StartTimeNs);
        const double DurationNs = static_cast<double>(m_Event.DurationNs);
        const double SurgeOffset =
            m_Event.PeakAmplitude * std::sin(m_Pi * (ElapsedNs / DurationNs));

        for (const std::size_t ChannelIndex : m_Event.TargetChannels)
        {
            OutDisturbance.Channels[ChannelIndex].AdditiveOffset += SurgeOffset;
        }
    }
}
