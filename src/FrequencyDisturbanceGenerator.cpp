#include "FrequencyDisturbanceGenerator.h"

FrequencyDisturbanceGenerator::FrequencyDisturbanceGenerator(
    const FrequencyDistortionEvent& rEvent)
    : m_rEvent(rEvent)
{
}


bool FrequencyDisturbanceGenerator::IsEventActive(std::uint64_t SimulationTimeNs) const
{
    return SimulationTimeNs >= m_rEvent.StartTimeNs &&
           (SimulationTimeNs - m_rEvent.StartTimeNs) < m_rEvent.DurationNs;
}


void FrequencyDisturbanceGenerator::EvaluateDisturbance(std::uint64_t SimulationTimeNs,
                                                        SampleDisturbance& rOutDisturbance)
{
    if (IsEventActive(SimulationTimeNs))
    {
        if (m_rEvent.Mode == FrequencyDistortionMode::Step)
        {
            rOutDisturbance.DeltaFrequencyHz += m_rEvent.DeltaFrequencyHz;
        }
        else
        {
            const double ElapsedNs = static_cast<double>(SimulationTimeNs - m_rEvent.StartTimeNs);
            const double DurationNs = static_cast<double>(m_rEvent.DurationNs);
            rOutDisturbance.DeltaFrequencyHz +=
                m_rEvent.DeltaFrequencyHz * (ElapsedNs / DurationNs);
        }
    }
}
