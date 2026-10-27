#include "config/Config.h"
#include "disturbance/IDisturbanceGenerator.h"
#include "mocks/MockDisturbanceGenerator.h"
#include "mocks/MockSignalGenerator.h"
#include "output/OutputGenerator.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <memory>
#include <vector>

namespace
{
SimulatorConfig CreateTwoChannelConfig(double GlobalOffset)
{
    SimulatorConfig Config{};
    Config.Offset = GlobalOffset;
    Config.Channels = {ChannelConfig{0, 0.0}, ChannelConfig{1, -120.0}};
    return Config;
}
} // namespace


TEST_CASE("OutputGenerator produces nominal channel samples with global offset when no disturbances exist",
          "[OutputGenerator]")
{
    using Catch::Matchers::WithinAbs;

    const SimulatorConfig Config = CreateTwoChannelConfig(2.5);
    MockSignalGenerator SignalGenerator({10.0, -20.0});
    const std::vector<std::unique_ptr<IDisturbanceGenerator>> DisturbanceGenerators{};

    OutputGenerator Generator(Config, SignalGenerator, DisturbanceGenerators);

    std::vector<double> OutChannelSamples;
    Generator.GenerateCompositeSample(1000U, OutChannelSamples);

    REQUIRE(SignalGenerator.GetGenerateSampleCallCount() == 1U);
    CHECK_THAT(SignalGenerator.GetLastReceivedDeltaFrequencyHz(), WithinAbs(0.0, 1e-12));

    REQUIRE(OutChannelSamples.size() == 2U);
    CHECK_THAT(OutChannelSamples[0], WithinAbs(12.5, 1e-12));
    CHECK_THAT(OutChannelSamples[1], WithinAbs(-17.5, 1e-12));
}


TEST_CASE("OutputGenerator applies per-channel amplitude scale, additive offset, and frequency delta",
          "[OutputGenerator]")
{
    using Catch::Matchers::WithinAbs;

    const SimulatorConfig Config = CreateTwoChannelConfig(1.0);
    MockSignalGenerator SignalGenerator({10.0, 20.0});

    std::vector<std::unique_ptr<IDisturbanceGenerator>> DisturbanceGenerators;
    DisturbanceGenerators.push_back(
        std::make_unique<MockDisturbanceGenerator>(100U, 500U, 0U, 0.5, 3.0, 1.5));

    OutputGenerator Generator(Config, SignalGenerator, DisturbanceGenerators);

    std::vector<double> OutChannelSamples;
    Generator.GenerateCompositeSample(200U, OutChannelSamples);

    CHECK_THAT(SignalGenerator.GetLastReceivedDeltaFrequencyHz(), WithinAbs(1.5, 1e-12));

    REQUIRE(OutChannelSamples.size() == 2U);
    CHECK_THAT(OutChannelSamples[0], WithinAbs(9.0, 1e-12));
    CHECK_THAT(OutChannelSamples[1], WithinAbs(21.0, 1e-12));
}


TEST_CASE("OutputGenerator composes multiple simultaneous disturbances and resets state across samples",
          "[OutputGenerator]")
{
    using Catch::Matchers::WithinAbs;

    const SimulatorConfig Config = CreateTwoChannelConfig(0.0);
    MockSignalGenerator SignalGenerator({100.0, 50.0});

    std::vector<std::unique_ptr<IDisturbanceGenerator>> DisturbanceGenerators;
    DisturbanceGenerators.push_back(
        std::make_unique<MockDisturbanceGenerator>(100U, 300U, 0U, 0.8, 5.0, 0.5));
    DisturbanceGenerators.push_back(
        std::make_unique<MockDisturbanceGenerator>(100U, 300U, 0U, 0.5, 10.0, -0.2));

    OutputGenerator Generator(Config, SignalGenerator, DisturbanceGenerators);

    std::vector<double> OutChannelSamples;

    Generator.GenerateCompositeSample(150U, OutChannelSamples);
    CHECK_THAT(SignalGenerator.GetLastReceivedDeltaFrequencyHz(), WithinAbs(0.3, 1e-12));
    REQUIRE(OutChannelSamples.size() == 2U);
    CHECK_THAT(OutChannelSamples[0], WithinAbs(55.0, 1e-12));
    CHECK_THAT(OutChannelSamples[1], WithinAbs(50.0, 1e-12));

    Generator.GenerateCompositeSample(400U, OutChannelSamples);
    CHECK_THAT(SignalGenerator.GetLastReceivedDeltaFrequencyHz(), WithinAbs(0.0, 1e-12));
    CHECK_THAT(OutChannelSamples[0], WithinAbs(100.0, 1e-12));
    CHECK_THAT(OutChannelSamples[1], WithinAbs(50.0, 1e-12));
}
