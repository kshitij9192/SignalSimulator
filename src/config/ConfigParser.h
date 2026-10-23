#pragma once

#include "config/Config.h"

#include <nlohmann/json_fwd.hpp>

#include <string_view>

class ConfigParser final
{
  public:
    static SimulatorConfig ParseConfigFile(const std::string& Path);

  private:
    using Json = nlohmann::json;

    [[noreturn]] static void ThrowConfigError(const std::string& Path, const std::string& Message);
    static const Json& FindKeyInJson(const Json& Object, std::string_view Key,
                                     std::string_view Path);
    static std::string ToStringValue(const Json& Value, const std::string& Path);
    static double ToNumericValue(const Json& Value, const std::string& Path);
    static std::uint64_t ToUnsignedIntegerValue(const Json& Value, const std::string& Path);
    static std::size_t ToSizeTValue(const Json& Value, const std::string& Path);
    static void RequireJsonFieldEquals(const Json& Object, const std::string& Key,
                                       const Json& Expected, const std::string& Path);
    static void ValidatePosixIpcName(const std::string& Name, const std::string& Path);
    static void ValidateSchemaReference(const Json& Root);
    static std::uint64_t ParseEventStartTime(const Json& Event, const std::string& Path);
    static std::uint64_t ParseEventDurationNanoseconds(const Json& Event, std::uint64_t StartTimeNs,
                                                       const std::string& Path);
    static std::vector<std::size_t> ParseEventTargetChannels(const Json& Event,
                                                             std::size_t ChannelCount,
                                                             const std::string& Path);
    static DisturbanceEvent ParseDisturbanceEvent(const Json& Event, std::size_t Index,
                                                  std::size_t ChannelCount, double FrequencyHz);
    static SurgeEvent ParseSurgeEvent(const Json& Event, std::uint64_t StartTimeNs,
                                      std::uint64_t DurationNs, std::size_t ChannelCount,
                                      const std::string& Path);
    static SagEvent ParseSagEvent(const Json& Event, std::uint64_t StartTimeNs,
                                  std::uint64_t DurationNs, std::size_t ChannelCount,
                                  const std::string& Path);
    static SwellEvent ParseSwellEvent(const Json& Event, std::uint64_t StartTimeNs,
                                      std::uint64_t DurationNs, std::size_t ChannelCount,
                                      const std::string& Path);
    static FrequencyDistortionEvent ParseFrequencyDistortionEvent(
        const Json& Event, std::uint64_t StartTimeNs, std::uint64_t DurationNs,
        double FrequencyHz, const std::string& Path);
    static void ParseSignalConfiguration(const Json& Root, SimulatorConfig& Config);
    static void ParseChannelConfigurations(const Json& Signal, SimulatorConfig& Config);
    static void ParseSamplingConfiguration(const Json& Root, SimulatorConfig& Config);
    static void ParseOutputConfiguration(const Json& Root, SimulatorConfig& Config);
    static void ParseOutputRepresentationAndLayout(const Json& Output, SimulatorConfig& Config);
    static void ParseAdcConfiguration(const Json& Output, SimulatorConfig& Config);
    static void ParseBufferConfiguration(const Json& Root, SimulatorConfig& Config);
    static void ValidateSamplePayloadSize(const SimulatorConfig& Config);
    static void ParseIpcConfiguration(const Json& Root, SimulatorConfig& Config);
    static void ValidateNominalSignalWithinAdcRange(const SimulatorConfig& Config);
    static void ParseDisturbanceTimeline(const Json& Root, SimulatorConfig& Config);
    static SimulatorConfig ExtractAndValidateJson(const Json& Root);
    static SimulatorConfig TryParsingJson(const Json& Document);
};