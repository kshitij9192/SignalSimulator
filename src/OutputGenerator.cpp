#include "OutputGenerator.h"

OutputGenerator::OutputGenerator(
    const SimulatorConfig& rConfig, ISignalGenerator& rSignalGenerator,
    const std::vector<std::unique_ptr<IDisturbanceGenerator>>& rDisturbanceGenerators)
    : m_rConfig(rConfig),
      m_rSignalGenerator(rSignalGenerator),
      m_rDisturbanceGenerators(rDisturbanceGenerators),
      m_BaseChannelSamples(rConfig.Channels.size(), 0.0)
{
    m_CurrentDisturbance.Channels.resize(rConfig.Channels.size());
}


void OutputGenerator::ResetCurrentDisturbance()
{
    m_CurrentDisturbance.DeltaFrequencyHz = 0.0;
    m_CurrentDisturbance.Channels.assign(m_rConfig.Channels.size(), ChannelDisturbance{});
}


void OutputGenerator::EvaluateAllDisturbances(std::uint64_t SimulationTimeNs)
{
    ResetCurrentDisturbance();

    for (const std::unique_ptr<IDisturbanceGenerator>& rGenerator : m_rDisturbanceGenerators)
    {
        rGenerator->EvaluateDisturbance(SimulationTimeNs, m_CurrentDisturbance);
    }
}


void OutputGenerator::GenerateCompositeSample(std::uint64_t SimulationTimeNs,
                                              std::vector<double>& rOutChannelSamples)
{
    EvaluateAllDisturbances(SimulationTimeNs);
    m_rSignalGenerator.GenerateSample(m_CurrentDisturbance.DeltaFrequencyHz, m_BaseChannelSamples);

    const std::size_t ChannelCount = m_rConfig.Channels.size();
    rOutChannelSamples.resize(ChannelCount);

    for (std::size_t ChannelIndex = 0; ChannelIndex < ChannelCount; ++ChannelIndex)
    {
        const ChannelDisturbance& rChannelDisturbance =
            m_CurrentDisturbance.Channels[ChannelIndex];
        rOutChannelSamples[ChannelIndex] =
            m_rConfig.Offset +
            (rChannelDisturbance.AmplitudeScale * m_BaseChannelSamples[ChannelIndex]) +
            rChannelDisturbance.AdditiveOffset;
    }
}
