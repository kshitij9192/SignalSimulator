#include "disturbance/FrequencyDisturbanceGenerator.h"

FrequencyDisturbanceGenerator::FrequencyDisturbanceGenerator(
    const FrequencyDistortionEvent& Event)
    : m_Event(Event)
{
}


bool FrequencyDisturbanceGenerator::IsEventActive(std::uint64_t SimulationTimeNs) const
{
    return SimulationTimeNs >= m_Event.StartTimeNs &&
           (SimulationTimeNs - m_Event.StartTimeNs) < m_Event.DurationNs;
}


void FrequencyDisturbanceGenerator::ApplyDisturbance(std::uint64_t SimulationTimeNs,
                                                     SampleDisturbance& OutDisturbance)
{
    if (IsEventActive(SimulationTimeNs))
    {
        if (m_Event.Mode == FrequencyDistortionMode::Step)
        {
            OutDisturbance.DeltaFrequencyHz += m_Event.DeltaFrequencyHz;
        }
        else
        {
            const double ElapsedNs = static_cast<double>(SimulationTimeNs - m_Event.StartTimeNs);
            const double DurationNs = static_cast<double>(m_Event.DurationNs);
            OutDisturbance.DeltaFrequencyHz +=
                m_Event.DeltaFrequencyHz * (ElapsedNs / DurationNs);
        }
    }
}
