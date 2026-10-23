#pragma once

#include "config/Config.h"
#include "signal/ISignalGenerator.h"

#include <vector>

class SineSignalGenerator final : public ISignalGenerator
{
  public:
    explicit SineSignalGenerator(const SimulatorConfig& Config);
    ~SineSignalGenerator() override = default;

    SineSignalGenerator(const SineSignalGenerator&) = delete;
    SineSignalGenerator& operator=(const SineSignalGenerator&) = delete;

    void GenerateSample(double DeltaFrequencyHz,
                        std::vector<double>& OutChannelSamples) override;
    double GetCurrentPhaseRadians() const override;

  private:
    static std::vector<double> ConvertChannelPhasesToRadians(
        const std::vector<ChannelConfig>& Channels);
    void AdvancePhase(double DeltaFrequencyHz);

    static constexpr double m_Pi = 3.14159265358979323846;
    static constexpr double m_TwoPi = 2.0 * m_Pi;
    static constexpr double m_DegreesToRadians = m_Pi / 180.0;

    const SimulatorConfig& m_Config;
    std::vector<double> m_ChannelPhaseOffsetsRadians;
    double m_CurrentPhaseRadians = 0.0;
};
