#include "SagDisturbanceGenerator.h"

SagDisturbanceGenerator::SagDisturbanceGenerator(const SagEvent& rEvent, std::size_t ChannelCount)
    : m_rEvent(rEvent), m_ChannelCount(ChannelCount)
{
}


bool SagDisturbanceGenerator::IsEventActive(std::uint64_t SimulationTimeNs) const
{
    return SimulationTimeNs >= m_rEvent.StartTimeNs &&
           (SimulationTimeNs - m_rEvent.StartTimeNs) < m_rEvent.DurationNs;
}


void SagDisturbanceGenerator::EvaluateDisturbance(std::uint64_t SimulationTimeNs,
                                                  SampleDisturbance& rOutDisturbance)
{
    if (rOutDisturbance.Channels.size() != m_ChannelCount)
    {
        rOutDisturbance.Channels.assign(m_ChannelCount, ChannelDisturbance{});
    }

    if (IsEventActive(SimulationTimeNs))
    {
        for (const std::size_t ChannelIndex : m_rEvent.TargetChannels)
        {
            rOutDisturbance.Channels[ChannelIndex].AmplitudeScale *= m_rEvent.ScalingFactor;
        }
    }
}
