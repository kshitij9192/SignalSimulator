#include "disturbance/SagDisturbanceGenerator.h"

SagDisturbanceGenerator::SagDisturbanceGenerator(const SagEvent& Event, std::size_t ChannelCount)
    : m_Event(Event), m_ChannelCount(ChannelCount)
{
}


bool SagDisturbanceGenerator::IsEventActive(std::uint64_t SimulationTimeNs) const
{
    return SimulationTimeNs >= m_Event.StartTimeNs &&
           (SimulationTimeNs - m_Event.StartTimeNs) < m_Event.DurationNs;
}


void SagDisturbanceGenerator::ApplyDisturbance(std::uint64_t SimulationTimeNs,
                                               SampleDisturbance& OutDisturbance)
{
    if (OutDisturbance.Channels.size() != m_ChannelCount)
    {
        OutDisturbance.Channels.assign(m_ChannelCount, ChannelDisturbance{});
    }

    if (IsEventActive(SimulationTimeNs))
    {
        for (const std::size_t ChannelIndex : m_Event.TargetChannels)
        {
            OutDisturbance.Channels[ChannelIndex].AmplitudeScale *= m_Event.ScalingFactor;
        }
    }
}
