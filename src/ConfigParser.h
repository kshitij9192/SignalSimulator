#pragma once

#include "Config.h"

#include <nlohmann/json_fwd.hpp>

#include <set>

class ConfigParser final
{
  public:
    static SimulatorConfig ParseConfigFile(const std::string& rPath);

  private:
    using Json = nlohmann::json;

    [[noreturn]] static void ThrowConfigError(const std::string& rPath, const std::string& rMessage);
    static const Json& FindKeyInJson(const Json& rObject, const std::string& rKey,
                                     const std::string& rPath);
    static std::string ToStringValue(const Json& rValue, const std::string& rPath);
    static double ToNumericValue(const Json& rValue, const std::string& rPath);
    static std::uint64_t ToUnsignedIntegerValue(const Json& rValue, const std::string& rPath);
    static std::size_t ToSizeTValue(const Json& rValue, const std::string& rPath);
    static void RequireJsonFieldEquals(const Json& rObject, const std::string& rKey,
                                       const Json& rExpected, const std::string& rPath);
    static void ValidatePosixIpcName(const std::string& rName, const std::string& rPath);
    static void ValidateSchemaReference(const Json& rRoot);
    static std::uint64_t ParseEventStartTime(const Json& rEvent, const std::string& rPath);
    static std::uint64_t ParseEventDurationNanoseconds(const Json& rEvent, std::uint64_t StartTimeNs,
                                                       const std::string& rPath);
    static std::vector<std::string> ParseEventTargetChannels(
        const Json& rEvent, const std::set<std::string>& rChannelIds, const std::string& rPath);
    static DisturbanceEvent ParseDisturbanceEvent(const Json& rEvent, std::size_t Index,
                                                  const std::set<std::string>& rChannelIds,
                                                  double FrequencyHz);
    static SurgeEvent ParseSurgeEvent(const Json& rEvent, std::uint64_t StartTimeNs,
                                      std::uint64_t DurationNs,
                                      const std::set<std::string>& rChannelIds,
                                      const std::string& rPath);
    static VoltageSagEvent ParseVoltageSagEvent(const Json& rEvent, std::uint64_t StartTimeNs,
                                                std::uint64_t DurationNs,
                                                const std::set<std::string>& rChannelIds,
                                                const std::string& rPath);
    static VoltageSwellEvent ParseVoltageSwellEvent(const Json& rEvent, std::uint64_t StartTimeNs,
                                                    std::uint64_t DurationNs,
                                                    const std::set<std::string>& rChannelIds,
                                                    const std::string& rPath);
    static FrequencyDistortionEvent ParseFrequencyDistortionEvent(
        const Json& rEvent, std::uint64_t StartTimeNs, std::uint64_t DurationNs,
        double FrequencyHz, const std::string& rPath);
    static void ParseSignalConfiguration(const Json& rRoot, SimulatorConfig& rConfig,
                                         std::set<std::string>& rChannelIds);
    static void ParseChannelConfigurations(const Json& rSignal, SimulatorConfig& rConfig,
                                           std::set<std::string>& rChannelIds);
    static void ParseSamplingConfiguration(const Json& rRoot, SimulatorConfig& rConfig);
    static void ParseOutputConfiguration(const Json& rRoot, SimulatorConfig& rConfig);
    static void ParseOutputRepresentationAndLayout(const Json& rOutput, SimulatorConfig& rConfig);
    static void ParseAdcConfiguration(const Json& rOutput, SimulatorConfig& rConfig);
    static void ParseBufferConfiguration(const Json& rRoot, SimulatorConfig& rConfig);
    static void ValidateSamplePayloadSize(const SimulatorConfig& rConfig);
    static void ParseIpcConfiguration(const Json& rRoot, SimulatorConfig& rConfig);
    static void ParseNotificationConfiguration(const Json& rIpc, SimulatorConfig& rConfig);
    static void ValidateNominalSignalWithinAdcRange(const SimulatorConfig& rConfig);
    static void ParseDisturbanceTimeline(const Json& rRoot, SimulatorConfig& rConfig,
                                         const std::set<std::string>& rChannelIds);
    static SimulatorConfig ExtractAndValidateJson(const Json& rRoot);
    static SimulatorConfig TryParsingJson(const Json& rDocument);
};