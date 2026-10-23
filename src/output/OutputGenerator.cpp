#include "output/OutputGenerator.h"

OutputGenerator::OutputGenerator(
    const SimulatorConfig& Config, ISignalGenerator& SignalGenerator,
    const std::vector<std::unique_ptr<IDisturbanceGenerator>>& DisturbanceGenerators)
    : m_Config(Config),
      m_SignalGenerator(SignalGenerator),
      m_DisturbanceGenerators(DisturbanceGenerators),
      m_BaseChannelSamples(Config.Channels.size(), 0.0)
{
    m_CurrentDisturbance.Channels.resize(Config.Channels.size());
}


void OutputGenerator::ResetCurrentDisturbance()
{
    m_CurrentDisturbance.DeltaFrequencyHz = 0.0;
    m_CurrentDisturbance.Channels.assign(m_Config.Channels.size(), ChannelDisturbance{});
}


void OutputGenerator::ApplyAllDisturbances(std::uint64_t SimulationTimeNs)
{
    ResetCurrentDisturbance();

    for (const std::unique_ptr<IDisturbanceGenerator>& Generator : m_DisturbanceGenerators)
    {
        Generator->ApplyDisturbance(SimulationTimeNs, m_CurrentDisturbance);
    }
}


void OutputGenerator::GenerateCompositeSample(std::uint64_t SimulationTimeNs,
                                              std::vector<double>& OutChannelSamples)
{
    ApplyAllDisturbances(SimulationTimeNs);
    m_SignalGenerator.GenerateSample(m_CurrentDisturbance.DeltaFrequencyHz, m_BaseChannelSamples);

    const std::size_t ChannelCount = m_Config.Channels.size();
    OutChannelSamples.resize(ChannelCount);

    for (std::size_t ChannelIndex = 0; ChannelIndex < ChannelCount; ++ChannelIndex)
    {
        const ChannelDisturbance& CurrentChannelDisturbance =
            m_CurrentDisturbance.Channels[ChannelIndex];
        OutChannelSamples[ChannelIndex] =
            m_Config.Offset +
            (CurrentChannelDisturbance.AmplitudeScale * m_BaseChannelSamples[ChannelIndex]) +
            CurrentChannelDisturbance.AdditiveOffset;
    }
}
