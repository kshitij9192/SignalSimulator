#include "config/Config.h"
#include "config/ConfigParser.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

class SampleConfigTestFixture
{
  protected:
    const SimulatorConfig m_Config = ConfigParser::ParseConfigFile("config/sample_config.json");
};


TEST_CASE_METHOD(SampleConfigTestFixture, "ConfigParser parses sample_config.json into SimulatorConfig", "[ConfigParser]")
{
    using Catch::Matchers::WithinAbs;

    SECTION("Signal configuration matches expected values")
    {
        CHECK_THAT(m_Config.FrequencyHz, WithinAbs(50.0, 1e-12));
        CHECK_THAT(m_Config.NominalAmplitude, WithinAbs(3.3, 1e-12));
        CHECK_THAT(m_Config.Offset, WithinAbs(0.0, 1e-12));

        REQUIRE(m_Config.Channels.size() == 3);
        CHECK(m_Config.Channels[0].Id == 0);
        CHECK_THAT(m_Config.Channels[0].PhaseDegrees, WithinAbs(0.0, 1e-12));
        CHECK(m_Config.Channels[1].Id == 1);
        CHECK_THAT(m_Config.Channels[1].PhaseDegrees, WithinAbs(120.0, 1e-12));
        CHECK(m_Config.Channels[2].Id == 2);
        CHECK_THAT(m_Config.Channels[2].PhaseDegrees, WithinAbs(240.0, 1e-12));
    }

    SECTION("Sampling configuration matches expected values")
    {
        REQUIRE(m_Config.SamplesPerCycle.has_value());
        CHECK(m_Config.SamplesPerCycle.value() == 128);
        CHECK_THAT(m_Config.SampleRateHz, WithinAbs(6400.0, 1e-12));
    }

    SECTION("Output and ADC configuration matches expected values")
    {
        CHECK(m_Config.Representation == SampleRepresentation::AdcInteger);
        CHECK(m_Config.Layout == ChannelDataLayout::Interleaved);

        REQUIRE(m_Config.Adc.has_value());
        const AdcConfig& Adc = m_Config.Adc.value();
        CHECK_THAT(Adc.ReferenceAmplitude, WithinAbs(5.0, 1e-12));
        CHECK(Adc.Polarity == AdcPolarity::Bipolar);
        CHECK(Adc.ResolutionBits == 16);
        CHECK(Adc.GetBytesPerSample() == 2);
        CHECK(m_Config.GetBytesPerSample() == 2);
    }

    SECTION("Buffer configuration matches expected values")
    {
        CHECK(m_Config.SlotCount == 4);
        CHECK(m_Config.BlockSizeSamplesPerChannel == 32);
    }

    SECTION("IPC configuration matches expected values")
    {
        CHECK(m_Config.SharedMemoryName == "/sigsim_mem");
        CHECK(m_Config.SemaphoreName == "/sigsim_data_ready");
    }

    SECTION("Disturbance timeline matches expected events")
    {
        REQUIRE(m_Config.DisturbanceTimeline.size() == 4);

        REQUIRE(std::holds_alternative<SurgeEvent>(m_Config.DisturbanceTimeline[0]));
        const auto& Surge = std::get<SurgeEvent>(m_Config.DisturbanceTimeline[0]);
        CHECK(Surge.StartTimeNs == 100000000ULL);
        CHECK(Surge.DurationNs == 2000000ULL);
        CHECK(Surge.TargetChannels == std::vector<std::size_t>{0});
        CHECK_THAT(Surge.PeakAmplitude, WithinAbs(1.5, 1e-12));

        REQUIRE(std::holds_alternative<SagEvent>(m_Config.DisturbanceTimeline[1]));
        const auto& Sag = std::get<SagEvent>(m_Config.DisturbanceTimeline[1]);
        CHECK(Sag.StartTimeNs == 200000000ULL);
        CHECK(Sag.DurationNs == 100000000ULL);
        CHECK(Sag.TargetChannels == std::vector<std::size_t>{1});
        CHECK_THAT(Sag.ScalingFactor, WithinAbs(0.65, 1e-12));

        REQUIRE(std::holds_alternative<SwellEvent>(m_Config.DisturbanceTimeline[2]));
        const auto& Swell = std::get<SwellEvent>(m_Config.DisturbanceTimeline[2]);
        CHECK(Swell.StartTimeNs == 400000000ULL);
        CHECK(Swell.DurationNs == 100000000ULL);
        CHECK(Swell.TargetChannels == std::vector<std::size_t>{0, 1, 2});
        CHECK_THAT(Swell.ScalingFactor, WithinAbs(1.25, 1e-12));

        REQUIRE(std::holds_alternative<FrequencyDistortionEvent>(m_Config.DisturbanceTimeline[3]));
        const auto& FrequencyEvent =
            std::get<FrequencyDistortionEvent>(m_Config.DisturbanceTimeline[3]);
        CHECK(FrequencyEvent.Mode == FrequencyDistortionMode::Step);
        CHECK(FrequencyEvent.StartTimeNs == 600000000ULL);
        CHECK(FrequencyEvent.DurationNs == 200000000ULL);
        CHECK_THAT(FrequencyEvent.DeltaFrequencyHz, WithinAbs(5.0, 1e-12));
    }
}
