#include "Config.h"
#include "ConfigParser.h"

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
        CHECK(*m_Config.SamplesPerCycle == 128);
        CHECK_THAT(m_Config.SampleRateHz, WithinAbs(6400.0, 1e-12));
    }

    SECTION("Output and ADC configuration matches expected values")
    {
        CHECK(m_Config.Representation == SampleRepresentation::AdcInteger);
        CHECK(m_Config.Layout == ChannelDataLayout::Interleaved);

        REQUIRE(m_Config.Adc.has_value());
        CHECK_THAT(m_Config.Adc->ReferenceAmplitude, WithinAbs(5.0, 1e-12));
        CHECK(m_Config.Adc->Polarity == AdcPolarity::Bipolar);
        CHECK(m_Config.Adc->ResolutionBits == 16);
        CHECK(m_Config.Adc->GetBytesPerSample() == 2);
        CHECK(m_Config.GetBytesPerSample() == 2);
    }

    SECTION("Buffer configuration matches expected values")
    {
        CHECK(m_Config.SlotCount == 4);
        CHECK(m_Config.BlockSizeSamplesPerChannel == 32);
    }

    SECTION("IPC configuration matches expected values")
    {
        CHECK(m_Config.SharedMemoryName == "/signalsimulator_adc");
        CHECK(m_Config.NotificationType == "posix_named_semaphore");
        CHECK(m_Config.NotificationName == "/signalsimulator_adc_ready");
        CHECK(m_Config.NotificationInitialValue == 0);
    }

    SECTION("Disturbance timeline matches expected events")
    {
        REQUIRE(m_Config.DisturbanceTimeline.size() == 4);

        REQUIRE(std::holds_alternative<SurgeEvent>(m_Config.DisturbanceTimeline[0]));
        const auto& rSurgeEvent = std::get<SurgeEvent>(m_Config.DisturbanceTimeline[0]);
        CHECK(rSurgeEvent.StartTimeNs == 100000000ULL);
        CHECK(rSurgeEvent.DurationNs == 2000000ULL);
        CHECK(rSurgeEvent.TargetChannels == std::vector<std::size_t>{0});
        CHECK_THAT(rSurgeEvent.PeakAmplitude, WithinAbs(1.5, 1e-12));

        REQUIRE(std::holds_alternative<SagEvent>(m_Config.DisturbanceTimeline[1]));
        const auto& rSagEvent = std::get<SagEvent>(m_Config.DisturbanceTimeline[1]);
        CHECK(rSagEvent.StartTimeNs == 200000000ULL);
        CHECK(rSagEvent.DurationNs == 100000000ULL);
        CHECK(rSagEvent.TargetChannels == std::vector<std::size_t>{1});
        CHECK_THAT(rSagEvent.ScalingFactor, WithinAbs(0.65, 1e-12));

        REQUIRE(std::holds_alternative<SwellEvent>(m_Config.DisturbanceTimeline[2]));
        const auto& rSwellEvent = std::get<SwellEvent>(m_Config.DisturbanceTimeline[2]);
        CHECK(rSwellEvent.StartTimeNs == 400000000ULL);
        CHECK(rSwellEvent.DurationNs == 100000000ULL);
        CHECK(rSwellEvent.TargetChannels == std::vector<std::size_t>{0, 1, 2});
        CHECK_THAT(rSwellEvent.ScalingFactor, WithinAbs(1.25, 1e-12));

        REQUIRE(std::holds_alternative<FrequencyDistortionEvent>(m_Config.DisturbanceTimeline[3]));
        const auto& rFrequencyEvent =
            std::get<FrequencyDistortionEvent>(m_Config.DisturbanceTimeline[3]);
        CHECK(rFrequencyEvent.Mode == FrequencyDistortionMode::Step);
        CHECK(rFrequencyEvent.StartTimeNs == 600000000ULL);
        CHECK(rFrequencyEvent.DurationNs == 200000000ULL);
        CHECK_THAT(rFrequencyEvent.DeltaFrequencyHz, WithinAbs(5.0, 1e-12));
    }
}
