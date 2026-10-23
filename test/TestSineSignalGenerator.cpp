#include "config/Config.h"
#include "signal/SineSignalGenerator.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <vector>

constexpr double Pi = 3.14159265358979323846;
constexpr double TwoPi = 2.0 * Pi;

class SineSignalGeneratorTestFixture
{
  protected:
    SineSignalGeneratorTestFixture()
    {
        m_Config.FrequencyHz = 50.0;
        m_Config.NominalAmplitude = 3.3;
        m_Config.Offset = 0.0;
        m_Config.SamplesPerCycle = 128;
        m_Config.SampleRateHz = 6400.0;
        m_Config.Channels = {
            ChannelConfig{0, 0.0},
            ChannelConfig{1, 120.0},
            ChannelConfig{2, 240.0},
        };
    }

    SimulatorConfig m_Config;
};


TEST_CASE_METHOD(SineSignalGeneratorTestFixture,
                 "SineSignalGenerator synthesizes multi-channel sinusoidal waveforms",
                 "[SineSignalGenerator]")
{
    using Catch::Matchers::WithinAbs;

    SECTION("Initial sample reflects configured three-phase displacements and balanced sum")
    {
        SineSignalGenerator Generator(m_Config);
        CHECK_THAT(Generator.GetCurrentPhaseRadians(), WithinAbs(0.0, 1e-12));

        std::vector<double> ChannelSamples;
        Generator.GenerateSample(0.0, ChannelSamples);

        REQUIRE(ChannelSamples.size() == 3);
        const double ExpectedMagnitude = m_Config.NominalAmplitude * (std::sqrt(3.0) / 2.0);
        CHECK_THAT(ChannelSamples[0], WithinAbs(0.0, 1e-12));
        CHECK_THAT(ChannelSamples[1], WithinAbs(ExpectedMagnitude, 1e-12));
        CHECK_THAT(ChannelSamples[2], WithinAbs(-ExpectedMagnitude, 1e-12));

        for (std::size_t SampleIndex = 1; SampleIndex < 128; ++SampleIndex)
        {
            Generator.GenerateSample(0.0, ChannelSamples);
            const double ThreePhaseSum =
                ChannelSamples[0] + ChannelSamples[1] + ChannelSamples[2];
            CHECK_THAT(ThreePhaseSum, WithinAbs(0.0, 1e-11));
        }
    }

    SECTION("Hits cardinal waveform points across a cycle and wraps phase after one full period")
    {
        SineSignalGenerator Generator(m_Config);
        std::vector<double> ChannelSamples;
        std::vector<double> ChannelZeroCycle;
        ChannelZeroCycle.reserve(129);

        for (std::size_t SampleIndex = 0; SampleIndex <= 128; ++SampleIndex)
        {
            Generator.GenerateSample(0.0, ChannelSamples);
            ChannelZeroCycle.push_back(ChannelSamples[0]);
        }

        CHECK_THAT(ChannelZeroCycle[0], WithinAbs(0.0, 1e-12));
        CHECK_THAT(ChannelZeroCycle[32], WithinAbs(m_Config.NominalAmplitude, 1e-12));
        CHECK_THAT(ChannelZeroCycle[64], WithinAbs(0.0, 1e-12));
        CHECK_THAT(ChannelZeroCycle[96], WithinAbs(-m_Config.NominalAmplitude, 1e-12));
        CHECK_THAT(ChannelZeroCycle[128], WithinAbs(0.0, 1e-12));
    }

    SECTION("Maintains bounded phase in [0, 2*pi) and accuracy over 1000 cycles")
    {
        SineSignalGenerator Generator(m_Config);
        std::vector<double> ChannelSamples;
        constexpr std::size_t TotalCycles = 1000;
        constexpr std::size_t SamplesPerCycle = 128;

        for (std::size_t Cycle = 0; Cycle < TotalCycles; ++Cycle)
        {
            for (std::size_t SampleIndex = 0; SampleIndex < SamplesPerCycle; ++SampleIndex)
            {
                const double PhaseBeforeSample = Generator.GetCurrentPhaseRadians();
                CHECK(PhaseBeforeSample >= 0.0);
                CHECK(PhaseBeforeSample < TwoPi);
                Generator.GenerateSample(0.0, ChannelSamples);
            }
        }

        const double WrappedPhaseError =
            std::fmod(Generator.GetCurrentPhaseRadians() + Pi, TwoPi) - Pi;
        CHECK_THAT(WrappedPhaseError, WithinAbs(0.0, 1e-9));
        Generator.GenerateSample(0.0, ChannelSamples);
        CHECK_THAT(ChannelSamples[0], WithinAbs(0.0, 1e-9));
    }

    SECTION("Preserves phase continuity across positive and negative frequency steps")
    {
        SineSignalGenerator Generator(m_Config);
        std::vector<double> ChannelSamples;

        for (std::size_t SampleIndex = 0; SampleIndex < 19; ++SampleIndex)
        {
            Generator.GenerateSample(0.0, ChannelSamples);
        }

        const double PhaseAtTransition = Generator.GetCurrentPhaseRadians();
        constexpr double PositiveDeltaFrequencyHz = 5.0;
        Generator.GenerateSample(PositiveDeltaFrequencyHz, ChannelSamples);

        CHECK_THAT(ChannelSamples[0],
                   WithinAbs(m_Config.NominalAmplitude * std::sin(PhaseAtTransition), 1e-12));

        const double ExpectedPhaseStepUp =
            (TwoPi * (m_Config.FrequencyHz + PositiveDeltaFrequencyHz)) / m_Config.SampleRateHz;
        CHECK_THAT(Generator.GetCurrentPhaseRadians(),
                   WithinAbs(std::fmod(PhaseAtTransition + ExpectedPhaseStepUp, TwoPi), 1e-12));

        const double PhaseBeforeStepDown = Generator.GetCurrentPhaseRadians();
        constexpr double NegativeDeltaFrequencyHz = -10.0;
        Generator.GenerateSample(NegativeDeltaFrequencyHz, ChannelSamples);

        CHECK_THAT(ChannelSamples[0],
                   WithinAbs(m_Config.NominalAmplitude * std::sin(PhaseBeforeStepDown), 1e-12));

        const double ExpectedPhaseStepDown =
            (TwoPi * (m_Config.FrequencyHz + NegativeDeltaFrequencyHz)) / m_Config.SampleRateHz;
        CHECK_THAT(
            Generator.GetCurrentPhaseRadians(),
            WithinAbs(std::fmod(PhaseBeforeStepDown + ExpectedPhaseStepDown, TwoPi), 1e-12));
    }

    SECTION("Supports zero amplitude and arbitrary channel counts")
    {
        m_Config.NominalAmplitude = 0.0;
        m_Config.Channels = {
            ChannelConfig{0, 0.0},
            ChannelConfig{1, 60.0},
            ChannelConfig{2, 120.0},
            ChannelConfig{3, 180.0},
            ChannelConfig{4, 240.0},
            ChannelConfig{5, 300.0},
        };

        SineSignalGenerator Generator(m_Config);
        std::vector<double> ChannelSamples;
        Generator.GenerateSample(0.0, ChannelSamples);

        REQUIRE(ChannelSamples.size() == 6);
        for (const double SampleValue : ChannelSamples)
        {
            CHECK_THAT(SampleValue, WithinAbs(0.0, 1e-12));
        }
        CHECK(Generator.GetCurrentPhaseRadians() > 0.0);
    }
}
