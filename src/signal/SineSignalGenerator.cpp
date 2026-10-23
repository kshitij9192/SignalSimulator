#include "signal/SineSignalGenerator.h"

#include <cmath>

SineSignalGenerator::SineSignalGenerator(const SimulatorConfig& Config)
    : m_Config(Config),
      m_ChannelPhaseOffsetsRadians(ConvertChannelPhasesToRadians(Config.Channels))
{
}


std::vector<double> SineSignalGenerator::ConvertChannelPhasesToRadians(
    const std::vector<ChannelConfig>& Channels)
{
    std::vector<double> PhaseOffsetsRadians;
    PhaseOffsetsRadians.reserve(Channels.size());

    for (const ChannelConfig& Channel : Channels)
    {
        PhaseOffsetsRadians.push_back(Channel.PhaseDegrees * m_DegreesToRadians);
    }

    return PhaseOffsetsRadians;
}


void SineSignalGenerator::AdvancePhase(double DeltaFrequencyHz)
{
    const double InstantaneousFrequencyHz = m_Config.FrequencyHz + DeltaFrequencyHz;
    const double PhaseIncrementRadians =
        (m_TwoPi * InstantaneousFrequencyHz) / m_Config.SampleRateHz;

    m_CurrentPhaseRadians = std::fmod(m_CurrentPhaseRadians + PhaseIncrementRadians, m_TwoPi);
    if (m_CurrentPhaseRadians < 0.0)
    {
        m_CurrentPhaseRadians += m_TwoPi;
    }
}


void SineSignalGenerator::GenerateSample(double DeltaFrequencyHz,
                                         std::vector<double>& OutChannelSamples)
{
    const std::size_t ChannelCount = m_ChannelPhaseOffsetsRadians.size();
    OutChannelSamples.resize(ChannelCount);

    for (std::size_t ChannelIndex = 0; ChannelIndex < ChannelCount; ++ChannelIndex)
    {
        const double ChannelPhaseRadians =
            m_CurrentPhaseRadians + m_ChannelPhaseOffsetsRadians[ChannelIndex];
        OutChannelSamples[ChannelIndex] =
            m_Config.NominalAmplitude * std::sin(ChannelPhaseRadians);
    }

    AdvancePhase(DeltaFrequencyHz);
}


double SineSignalGenerator::GetCurrentPhaseRadians() const
{
    return m_CurrentPhaseRadians;
}
