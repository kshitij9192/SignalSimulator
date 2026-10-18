#pragma once

#include "Config.h"
#include "ISignalGenerator.h"

#include <vector>

class SineSignalGenerator final : public ISignalGenerator
{
  public:
    explicit SineSignalGenerator(const SimulatorConfig& rConfig);
    ~SineSignalGenerator() override = default;

    SineSignalGenerator(const SineSignalGenerator&) = delete;
    SineSignalGenerator& operator=(const SineSignalGenerator&) = delete;

    void GenerateSample(double DeltaFrequencyHz,
                        std::vector<double>& rOutChannelSamples) override;
    double GetCurrentPhaseRadians() const override;

  private:
    static std::vector<double> ConvertChannelPhasesToRadians(
        const std::vector<ChannelConfig>& rChannels);
    void AdvancePhase(double DeltaFrequencyHz);

    const SimulatorConfig& m_rConfig;
    std::vector<double> m_ChannelPhaseOffsetsRadians;
    double m_CurrentPhaseRadians = 0.0;
};
