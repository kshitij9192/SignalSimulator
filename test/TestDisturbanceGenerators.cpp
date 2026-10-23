#include "config/Config.h"
#include "disturbance/FrequencyDisturbanceGenerator.h"
#include "disturbance/IDisturbanceGenerator.h"
#include "disturbance/SagDisturbanceGenerator.h"
#include "disturbance/SurgeDisturbanceGenerator.h"
#include "disturbance/SwellDisturbanceGenerator.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>

constexpr std::size_t DefaultChannelCount = 3;

SampleDisturbance CreateNominalDisturbance(std::size_t ChannelCount = DefaultChannelCount)
{
    SampleDisturbance Disturbance;
    Disturbance.DeltaFrequencyHz = 0.0;
    Disturbance.Channels.assign(ChannelCount, ChannelDisturbance{});
    return Disturbance;
}

TEST_CASE("SurgeDisturbanceGenerator applies half-sine additive transient on targeted channels",
          "[DisturbanceGenerator][Surge]")
{
    using Catch::Matchers::WithinAbs;

    constexpr std::uint64_t StartTimeNs = 100'000'000ULL;
    constexpr std::uint64_t DurationNs = 2'000'000ULL;
    const SurgeEvent PositiveSurge{StartTimeNs, DurationNs, {0, 2}, 1.5};
    SurgeDisturbanceGenerator Generator(PositiveSurge, DefaultChannelCount);

    SECTION("Inactive before start and at or after end timestamp")
    {
        SampleDisturbance BeforeStart = CreateNominalDisturbance();
        Generator.ApplyDisturbance(StartTimeNs - 1, BeforeStart);
        CHECK_THAT(BeforeStart.Channels[0].AdditiveOffset, WithinAbs(0.0, 1e-12));
        CHECK_THAT(BeforeStart.Channels[1].AdditiveOffset, WithinAbs(0.0, 1e-12));
        CHECK_THAT(BeforeStart.Channels[2].AdditiveOffset, WithinAbs(0.0, 1e-12));

        SampleDisturbance AtEnd = CreateNominalDisturbance();
        Generator.ApplyDisturbance(StartTimeNs + DurationNs, AtEnd);
        CHECK_THAT(AtEnd.Channels[0].AdditiveOffset, WithinAbs(0.0, 1e-12));
        CHECK_THAT(AtEnd.Channels[1].AdditiveOffset, WithinAbs(0.0, 1e-12));
        CHECK_THAT(AtEnd.Channels[2].AdditiveOffset, WithinAbs(0.0, 1e-12));
    }

    SECTION("Follows half-sine envelope across active window and isolates target channels")
    {
        SampleDisturbance AtStart = CreateNominalDisturbance();
        Generator.ApplyDisturbance(StartTimeNs, AtStart);
        CHECK_THAT(AtStart.Channels[0].AdditiveOffset, WithinAbs(0.0, 1e-12));

        const double QuarterExpected = 1.5 * (std::sqrt(2.0) / 2.0);
        SampleDisturbance AtQuarter = CreateNominalDisturbance();
        Generator.ApplyDisturbance(StartTimeNs + (DurationNs / 4), AtQuarter);
        CHECK_THAT(AtQuarter.Channels[0].AdditiveOffset, WithinAbs(QuarterExpected, 1e-12));
        CHECK_THAT(AtQuarter.Channels[1].AdditiveOffset, WithinAbs(0.0, 1e-12));
        CHECK_THAT(AtQuarter.Channels[2].AdditiveOffset, WithinAbs(QuarterExpected, 1e-12));
        CHECK_THAT(AtQuarter.Channels[0].AmplitudeScale, WithinAbs(1.0, 1e-12));
        CHECK_THAT(AtQuarter.DeltaFrequencyHz, WithinAbs(0.0, 1e-12));

        SampleDisturbance AtMidpoint = CreateNominalDisturbance();
        Generator.ApplyDisturbance(StartTimeNs + (DurationNs / 2), AtMidpoint);
        CHECK_THAT(AtMidpoint.Channels[0].AdditiveOffset, WithinAbs(1.5, 1e-12));
        CHECK_THAT(AtMidpoint.Channels[1].AdditiveOffset, WithinAbs(0.0, 1e-12));
        CHECK_THAT(AtMidpoint.Channels[2].AdditiveOffset, WithinAbs(1.5, 1e-12));

        SampleDisturbance AtThreeQuarters = CreateNominalDisturbance();
        Generator.ApplyDisturbance(StartTimeNs + ((3 * DurationNs) / 4), AtThreeQuarters);
        CHECK_THAT(AtThreeQuarters.Channels[0].AdditiveOffset, WithinAbs(QuarterExpected, 1e-12));
        CHECK_THAT(AtThreeQuarters.Channels[1].AdditiveOffset, WithinAbs(0.0, 1e-12));
        CHECK_THAT(AtThreeQuarters.Channels[2].AdditiveOffset, WithinAbs(QuarterExpected, 1e-12));
    }

    SECTION("Supports negative peak amplitude")
    {
        const SurgeEvent NegativeSurge{StartTimeNs, DurationNs, {1}, -2.0};
        SurgeDisturbanceGenerator NegativeGenerator(NegativeSurge, DefaultChannelCount);

        SampleDisturbance AtMidpoint = CreateNominalDisturbance();
        NegativeGenerator.ApplyDisturbance(StartTimeNs + (DurationNs / 2), AtMidpoint);
        CHECK_THAT(AtMidpoint.Channels[0].AdditiveOffset, WithinAbs(0.0, 1e-12));
        CHECK_THAT(AtMidpoint.Channels[1].AdditiveOffset, WithinAbs(-2.0, 1e-12));
        CHECK_THAT(AtMidpoint.Channels[2].AdditiveOffset, WithinAbs(0.0, 1e-12));
    }
}


TEST_CASE("SagDisturbanceGenerator scales amplitude on targeted channels",
          "[DisturbanceGenerator][Sag]")
{
    using Catch::Matchers::WithinAbs;

    constexpr std::uint64_t StartTimeNs = 200'000'000ULL;
    constexpr std::uint64_t DurationNs = 100'000'000ULL;

    SECTION("Applies partial sag within active window and restores nominal outside")
    {
        const SagEvent PartialSag{StartTimeNs, DurationNs, {1}, 0.65};
        SagDisturbanceGenerator Generator(PartialSag, DefaultChannelCount);

        SampleDisturbance BeforeStart = CreateNominalDisturbance();
        Generator.ApplyDisturbance(StartTimeNs - 1, BeforeStart);
        CHECK_THAT(BeforeStart.Channels[1].AmplitudeScale, WithinAbs(1.0, 1e-12));

        SampleDisturbance AtStart = CreateNominalDisturbance();
        Generator.ApplyDisturbance(StartTimeNs, AtStart);
        CHECK_THAT(AtStart.Channels[0].AmplitudeScale, WithinAbs(1.0, 1e-12));
        CHECK_THAT(AtStart.Channels[1].AmplitudeScale, WithinAbs(0.65, 1e-12));
        CHECK_THAT(AtStart.Channels[2].AmplitudeScale, WithinAbs(1.0, 1e-12));
        CHECK_THAT(AtStart.Channels[1].AdditiveOffset, WithinAbs(0.0, 1e-12));
        CHECK_THAT(AtStart.DeltaFrequencyHz, WithinAbs(0.0, 1e-12));

        SampleDisturbance BeforeEnd = CreateNominalDisturbance();
        Generator.ApplyDisturbance(StartTimeNs + DurationNs - 1, BeforeEnd);
        CHECK_THAT(BeforeEnd.Channels[1].AmplitudeScale, WithinAbs(0.65, 1e-12));

        SampleDisturbance AtEnd = CreateNominalDisturbance();
        Generator.ApplyDisturbance(StartTimeNs + DurationNs, AtEnd);
        CHECK_THAT(AtEnd.Channels[1].AmplitudeScale, WithinAbs(1.0, 1e-12));
    }

    SECTION("Supports complete dropout with scaling factor of zero")
    {
        const SagEvent ZeroSag{StartTimeNs, DurationNs, {0, 2}, 0.0};
        SagDisturbanceGenerator Generator(ZeroSag, DefaultChannelCount);

        SampleDisturbance DuringEvent = CreateNominalDisturbance();
        Generator.ApplyDisturbance(StartTimeNs + 1000, DuringEvent);
        CHECK_THAT(DuringEvent.Channels[0].AmplitudeScale, WithinAbs(0.0, 1e-12));
        CHECK_THAT(DuringEvent.Channels[1].AmplitudeScale, WithinAbs(1.0, 1e-12));
        CHECK_THAT(DuringEvent.Channels[2].AmplitudeScale, WithinAbs(0.0, 1e-12));
    }
}


TEST_CASE("SwellDisturbanceGenerator scales amplitude above unity on targeted channels",
          "[DisturbanceGenerator][Swell]")
{
    using Catch::Matchers::WithinAbs;

    constexpr std::uint64_t StartTimeNs = 400'000'000ULL;
    constexpr std::uint64_t DurationNs = 100'000'000ULL;
    const SwellEvent Swell{StartTimeNs, DurationNs, {0, 1, 2}, 1.25};
    SwellDisturbanceGenerator Generator(Swell, DefaultChannelCount);

    SampleDisturbance BeforeStart = CreateNominalDisturbance();
    Generator.ApplyDisturbance(StartTimeNs - 1, BeforeStart);
    CHECK_THAT(BeforeStart.Channels[0].AmplitudeScale, WithinAbs(1.0, 1e-12));

    SampleDisturbance DuringEvent = CreateNominalDisturbance();
    Generator.ApplyDisturbance(StartTimeNs + (DurationNs / 2), DuringEvent);
    CHECK_THAT(DuringEvent.Channels[0].AmplitudeScale, WithinAbs(1.25, 1e-12));
    CHECK_THAT(DuringEvent.Channels[1].AmplitudeScale, WithinAbs(1.25, 1e-12));
    CHECK_THAT(DuringEvent.Channels[2].AmplitudeScale, WithinAbs(1.25, 1e-12));
    CHECK_THAT(DuringEvent.Channels[0].AdditiveOffset, WithinAbs(0.0, 1e-12));
    CHECK_THAT(DuringEvent.DeltaFrequencyHz, WithinAbs(0.0, 1e-12));

    SampleDisturbance AtEnd = CreateNominalDisturbance();
    Generator.ApplyDisturbance(StartTimeNs + DurationNs, AtEnd);
    CHECK_THAT(AtEnd.Channels[0].AmplitudeScale, WithinAbs(1.0, 1e-12));
    CHECK_THAT(AtEnd.Channels[1].AmplitudeScale, WithinAbs(1.0, 1e-12));
    CHECK_THAT(AtEnd.Channels[2].AmplitudeScale, WithinAbs(1.0, 1e-12));
}


TEST_CASE("FrequencyDisturbanceGenerator applies step and ramp frequency offsets",
          "[DisturbanceGenerator][Frequency]")
{
    using Catch::Matchers::WithinAbs;

    constexpr std::uint64_t StartTimeNs = 600'000'000ULL;
    constexpr std::uint64_t DurationNs = 200'000'000ULL;

    SECTION("Step mode applies constant frequency offset across active window")
    {
        const FrequencyDistortionEvent StepEvent{
            FrequencyDistortionMode::Step, StartTimeNs, DurationNs, 5.0};
        FrequencyDisturbanceGenerator Generator(StepEvent);

        SampleDisturbance BeforeStart = CreateNominalDisturbance();
        Generator.ApplyDisturbance(StartTimeNs - 1, BeforeStart);
        CHECK_THAT(BeforeStart.DeltaFrequencyHz, WithinAbs(0.0, 1e-12));

        SampleDisturbance AtStart = CreateNominalDisturbance();
        Generator.ApplyDisturbance(StartTimeNs, AtStart);
        CHECK_THAT(AtStart.DeltaFrequencyHz, WithinAbs(5.0, 1e-12));

        SampleDisturbance BeforeEnd = CreateNominalDisturbance();
        Generator.ApplyDisturbance(StartTimeNs + DurationNs - 1, BeforeEnd);
        CHECK_THAT(BeforeEnd.DeltaFrequencyHz, WithinAbs(5.0, 1e-12));

        SampleDisturbance AtEnd = CreateNominalDisturbance();
        Generator.ApplyDisturbance(StartTimeNs + DurationNs, AtEnd);
        CHECK_THAT(AtEnd.DeltaFrequencyHz, WithinAbs(0.0, 1e-12));
    }

    SECTION("Ramp mode linearly interpolates frequency offset from zero to target")
    {
        const FrequencyDistortionEvent RampEvent{
            FrequencyDistortionMode::Ramp, StartTimeNs, DurationNs, -4.0};
        FrequencyDisturbanceGenerator Generator(RampEvent);

        SampleDisturbance AtStart = CreateNominalDisturbance();
        Generator.ApplyDisturbance(StartTimeNs, AtStart);
        CHECK_THAT(AtStart.DeltaFrequencyHz, WithinAbs(0.0, 1e-12));

        SampleDisturbance AtQuarter = CreateNominalDisturbance();
        Generator.ApplyDisturbance(StartTimeNs + (DurationNs / 4), AtQuarter);
        CHECK_THAT(AtQuarter.DeltaFrequencyHz, WithinAbs(-1.0, 1e-12));

        SampleDisturbance AtMidpoint = CreateNominalDisturbance();
        Generator.ApplyDisturbance(StartTimeNs + (DurationNs / 2), AtMidpoint);
        CHECK_THAT(AtMidpoint.DeltaFrequencyHz, WithinAbs(-2.0, 1e-12));

        SampleDisturbance AtThreeQuarters = CreateNominalDisturbance();
        Generator.ApplyDisturbance(StartTimeNs + ((3 * DurationNs) / 4), AtThreeQuarters);
        CHECK_THAT(AtThreeQuarters.DeltaFrequencyHz, WithinAbs(-3.0, 1e-12));

        SampleDisturbance AtEnd = CreateNominalDisturbance();
        Generator.ApplyDisturbance(StartTimeNs + DurationNs, AtEnd);
        CHECK_THAT(AtEnd.DeltaFrequencyHz, WithinAbs(0.0, 1e-12));
    }
}


TEST_CASE("Multiple disturbance generators compose onto the same SampleDisturbance",
          "[DisturbanceGenerator][Composition]")
{
    using Catch::Matchers::WithinAbs;

    constexpr std::uint64_t StartTimeNs = 100'000'000ULL;
    constexpr std::uint64_t DurationNs = 10'000'000ULL;

    const SurgeEvent Surge{StartTimeNs, DurationNs, {0}, 1.5};
    const SagEvent Sag{StartTimeNs, DurationNs, {0}, 0.5};
    const SwellEvent Swell{StartTimeNs, DurationNs, {2}, 1.2};
    const FrequencyDistortionEvent FrequencyStep{
        FrequencyDistortionMode::Step, StartTimeNs, DurationNs, 2.5};

    SurgeDisturbanceGenerator SurgeGen(Surge, DefaultChannelCount);
    SagDisturbanceGenerator SagGen(Sag, DefaultChannelCount);
    SwellDisturbanceGenerator SwellGen(Swell, DefaultChannelCount);
    FrequencyDisturbanceGenerator FrequencyGen(FrequencyStep);

    SampleDisturbance Composite = CreateNominalDisturbance();
    const std::uint64_t MidpointTimeNs = StartTimeNs + (DurationNs / 2);

    SurgeGen.ApplyDisturbance(MidpointTimeNs, Composite);
    SagGen.ApplyDisturbance(MidpointTimeNs, Composite);
    SwellGen.ApplyDisturbance(MidpointTimeNs, Composite);
    FrequencyGen.ApplyDisturbance(MidpointTimeNs, Composite);

    CHECK_THAT(Composite.DeltaFrequencyHz, WithinAbs(2.5, 1e-12));
    CHECK_THAT(Composite.Channels[0].AmplitudeScale, WithinAbs(0.5, 1e-12));
    CHECK_THAT(Composite.Channels[0].AdditiveOffset, WithinAbs(1.5, 1e-12));
    CHECK_THAT(Composite.Channels[1].AmplitudeScale, WithinAbs(1.0, 1e-12));
    CHECK_THAT(Composite.Channels[1].AdditiveOffset, WithinAbs(0.0, 1e-12));
    CHECK_THAT(Composite.Channels[2].AmplitudeScale, WithinAbs(1.2, 1e-12));
    CHECK_THAT(Composite.Channels[2].AdditiveOffset, WithinAbs(0.0, 1e-12));
}
