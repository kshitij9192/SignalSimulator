#include "SineSignalGenerator.h"

#include <cmath>

namespace
{
constexpr double Pi = 3.14159265358979323846;
constexpr double TwoPi = 2.0 * Pi;
constexpr double DegreesToRadians = Pi / 180.0;
} // namespace


SineSignalGenerator::SineSignalGenerator(const SimulatorConfig& rConfig)
    : m_rConfig(rConfig),
      m_ChannelPhaseOffsetsRadians(ConvertChannelPhasesToRadians(rConfig.Channels))
{
}


std::vector<double> SineSignalGenerator::ConvertChannelPhasesToRadians(
    const std::vector<ChannelConfig>& rChannels)
{
    std::vector<double> PhaseOffsetsRadians;
    PhaseOffsetsRadians.reserve(rChannels.size());

    for (const ChannelConfig& rChannel : rChannels)
    {
        PhaseOffsetsRadians.push_back(rChannel.PhaseDegrees * DegreesToRadians);
    }

    return PhaseOffsetsRadians;
}


void SineSignalGenerator::AdvancePhase(double DeltaFrequencyHz)
{
    const double InstantaneousFrequencyHz = m_rConfig.FrequencyHz + DeltaFrequencyHz;
    const double PhaseIncrementRadians =
        (TwoPi * InstantaneousFrequencyHz) / m_rConfig.SampleRateHz;

    m_CurrentPhaseRadians = std::fmod(m_CurrentPhaseRadians + PhaseIncrementRadians, TwoPi);
    if (m_CurrentPhaseRadians < 0.0)
    {
        m_CurrentPhaseRadians += TwoPi;
    }
}


void SineSignalGenerator::GenerateSample(double DeltaFrequencyHz,
                                         std::vector<double>& rOutChannelSamples)
{
    const std::size_t ChannelCount = m_ChannelPhaseOffsetsRadians.size();
    rOutChannelSamples.resize(ChannelCount);

    for (std::size_t ChannelIndex = 0; ChannelIndex < ChannelCount; ++ChannelIndex)
    {
        const double ChannelPhaseRadians =
            m_CurrentPhaseRadians + m_ChannelPhaseOffsetsRadians[ChannelIndex];
        rOutChannelSamples[ChannelIndex] =
            m_rConfig.NominalAmplitude * std::sin(ChannelPhaseRadians);
    }

    AdvancePhase(DeltaFrequencyHz);
}


double SineSignalGenerator::GetCurrentPhaseRadians() const
{
    return m_CurrentPhaseRadians;
}
