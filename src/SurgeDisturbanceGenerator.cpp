#include "SurgeDisturbanceGenerator.h"

#include <cmath>

namespace
{
constexpr double Pi = 3.14159265358979323846;
} // namespace


SurgeDisturbanceGenerator::SurgeDisturbanceGenerator(const SurgeEvent& rEvent,
                                                     std::size_t ChannelCount)
    : m_rEvent(rEvent), m_ChannelCount(ChannelCount)
{
}


bool SurgeDisturbanceGenerator::IsEventActive(std::uint64_t SimulationTimeNs) const
{
    return SimulationTimeNs >= m_rEvent.StartTimeNs &&
           (SimulationTimeNs - m_rEvent.StartTimeNs) < m_rEvent.DurationNs;
}


void SurgeDisturbanceGenerator::EvaluateDisturbance(std::uint64_t SimulationTimeNs,
                                                    SampleDisturbance& rOutDisturbance)
{
    if (rOutDisturbance.Channels.size() != m_ChannelCount)
    {
        rOutDisturbance.Channels.assign(m_ChannelCount, ChannelDisturbance{});
    }

    if (IsEventActive(SimulationTimeNs))
    {
        const double ElapsedNs = static_cast<double>(SimulationTimeNs - m_rEvent.StartTimeNs);
        const double DurationNs = static_cast<double>(m_rEvent.DurationNs);
        const double SurgeOffset =
            m_rEvent.PeakAmplitude * std::sin(Pi * (ElapsedNs / DurationNs));

        for (const std::size_t ChannelIndex : m_rEvent.TargetChannels)
        {
            rOutDisturbance.Channels[ChannelIndex].AdditiveOffset += SurgeOffset;
        }
    }
}
