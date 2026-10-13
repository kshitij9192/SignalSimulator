#include "ConfigParser.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <fstream>
#include <limits>
#include <set>
#include <stdexcept>

[[noreturn]] void ConfigParser::ThrowConfigError(const std::string& rPath, const std::string& rMessage)
{
    throw std::invalid_argument(rPath + ": " + rMessage);
}


const ConfigParser::Json& ConfigParser::FindKeyInJson(const Json& rObject,
                                                      const std::string& rKey,
                                                      const std::string& rPath)
{
    const auto It = rObject.find(rKey);
    if (It == rObject.end())
    {
        ThrowConfigError(rPath + "." + rKey, "required property is missing");
    }

    return *It;
}


std::string ConfigParser::ToStringValue(const Json& rValue, const std::string& rPath)
{
    if (!rValue.is_string())
    {
        ThrowConfigError(rPath, "expected a string");
    }

    return rValue.get<std::string>();
}


double ConfigParser::ToNumericValue(const Json& rValue, const std::string& rPath)
{
    if (!rValue.is_number())
    {
        ThrowConfigError(rPath, "expected a number");
    }

    return rValue.get<double>();
}


std::uint64_t ConfigParser::ToUnsignedIntegerValue(const Json& rValue, const std::string& rPath)
{
    if (rValue.is_number_unsigned())
    {
        return rValue.get<std::uint64_t>();
    }

    ThrowConfigError(rPath, "expected a non-negative integer");
}


std::size_t ConfigParser::ToSizeTValue(const Json& rValue, const std::string& rPath)
{
    const std::uint64_t Value = ToUnsignedIntegerValue(rValue, rPath);

    return static_cast<std::size_t>(Value);
}


void ConfigParser::RequireJsonFieldEquals(const Json& rObject, const std::string& rKey,
                                          const Json& rExpected, const std::string& rPath)
{
    const Json& Value = FindKeyInJson(rObject, rKey, rPath);
    if (Value != rExpected)
    {
        ThrowConfigError(rPath + "." + rKey, "has an unsupported value");
    }
}


void ConfigParser::ValidatePosixIpcName(const std::string& rName, const std::string& rPath)
{
    if (rName.size() < 2 || rName.front() != '/' || rName.find('/', 1) != std::string::npos ||
        rName.find('\0') != std::string::npos)
    {
        ThrowConfigError(rPath, "must begin with '/' and contain no other '/' characters");
    }
}


std::uint64_t ConfigParser::ParseEventStartTime(const Json& rEvent, const std::string& rPath)
{
    return ToUnsignedIntegerValue(FindKeyInJson(rEvent, "start_time_ns", rPath),
                                  rPath + ".start_time_ns");
}


std::uint64_t ConfigParser::ParseEventDurationNanoseconds(const Json& rEvent, std::uint64_t StartTimeNs,
                                                          const std::string& rPath)
{
    const std::uint64_t DurationNs =
        ToUnsignedIntegerValue(FindKeyInJson(rEvent, "duration_ns", rPath), rPath + ".duration_ns");
    if (DurationNs == 0)
    {
        ThrowConfigError(rPath + ".duration_ns", "must be greater than zero");
    }
    if ((StartTimeNs + DurationNs) > std::numeric_limits<std::uint64_t>::max())
    {
        ThrowConfigError(rPath, "start_time_ns plus duration_ns overflows");
    }

    return DurationNs;
}


std::vector<std::string> ConfigParser::ParseEventTargetChannels(const Json& rEvent,
                                                                const std::set<std::string>& rChannelIds,
                                                                const std::string& rPath)
{
    const Json& Targets = FindKeyInJson(rEvent, "target_channels", rPath);
    if (!Targets.is_array() || Targets.empty())
    {
        ThrowConfigError(rPath + ".target_channels", "expected a non-empty array");
    }

    std::set<std::string> UniqueTargets;
    std::vector<std::string> Result;
    for (std::size_t Index = 0; Index < Targets.size(); ++Index)
    {
        const std::string Target =
            ToStringValue(Targets[Index], rPath + ".target_channels[" + std::to_string(Index) + "]");
        if (Target.empty())
        {
            ThrowConfigError(rPath + ".target_channels[" + std::to_string(Index) + "]",
                             "must not be empty");
        }
        if (!UniqueTargets.insert(Target).second)
        {
            ThrowConfigError(rPath + ".target_channels", "channel IDs must be unique");
        }
        if (rChannelIds.find(Target) == rChannelIds.end())
        {
            ThrowConfigError(rPath + ".target_channels", "references unknown channel '" + Target + "'");
        }
        Result.push_back(Target);
    }
    return Result;
}


DisturbanceEvent ConfigParser::ParseDisturbanceEvent(const Json& rEvent, std::size_t Index,
                                                     const std::set<std::string>& rChannelIds,
                                                     double FrequencyHz)
{
    const std::string Path = "disturbance_timeline[" + std::to_string(Index) + "]";
    const std::string Type = ToStringValue(FindKeyInJson(rEvent, "type", Path), Path + ".type");
    const std::uint64_t StartTimeNs = ParseEventStartTime(rEvent, Path);
    const std::uint64_t DurationNs = ParseEventDurationNanoseconds(rEvent, StartTimeNs, Path);

    if (Type == "surge")
    {
        return ParseSurgeEvent(rEvent, StartTimeNs, DurationNs, rChannelIds, Path);
    }

    if (Type == "voltage_sag")
    {
        return ParseVoltageSagEvent(rEvent, StartTimeNs, DurationNs, rChannelIds, Path);
    }

    if (Type == "voltage_swell")
    {
        return ParseVoltageSwellEvent(rEvent, StartTimeNs, DurationNs, rChannelIds, Path);
    }

    if (Type == "frequency_distortion")
    {
        return ParseFrequencyDistortionEvent(rEvent, StartTimeNs, DurationNs, FrequencyHz, Path);
    }

    ThrowConfigError(Path + ".type", "must be 'surge', 'voltage_sag', 'voltage_swell', or 'frequency_distortion'");
}


SurgeEvent ConfigParser::ParseSurgeEvent(const Json& rEvent, std::uint64_t StartTimeNs,
                                         std::uint64_t DurationNs,
                                         const std::set<std::string>& rChannelIds,
                                         const std::string& rPath)
{
    RequireJsonFieldEquals(rEvent, "shape", "half_sine", rPath);
    const double PeakAmplitudeV = ToNumericValue(
        FindKeyInJson(rEvent, "peak_amplitude_v", rPath), rPath + ".peak_amplitude_v");
    if (PeakAmplitudeV <= 0.0)
    {
        ThrowConfigError(rPath + ".peak_amplitude_v", "must be greater than zero");
    }
    return SurgeEvent{StartTimeNs, DurationNs,
                      ParseEventTargetChannels(rEvent, rChannelIds, rPath), PeakAmplitudeV};
}


VoltageSagEvent ConfigParser::ParseVoltageSagEvent(const Json& rEvent,
                                                   std::uint64_t StartTimeNs,
                                                   std::uint64_t DurationNs,
                                                   const std::set<std::string>& rChannelIds,
                                                   const std::string& rPath)
{
    const double ScalingFactor = ToNumericValue(
        FindKeyInJson(rEvent, "scaling_factor", rPath), rPath + ".scaling_factor");
    if (ScalingFactor <= 0.0 || ScalingFactor >= 1.0)
    {
        ThrowConfigError(rPath + ".scaling_factor", "must be greater than 0 and less than 1");
    }
    return VoltageSagEvent{StartTimeNs, DurationNs,
                           ParseEventTargetChannels(rEvent, rChannelIds, rPath), ScalingFactor};
}


VoltageSwellEvent ConfigParser::ParseVoltageSwellEvent(const Json& rEvent,
                                                       std::uint64_t StartTimeNs,
                                                       std::uint64_t DurationNs,
                                                       const std::set<std::string>& rChannelIds,
                                                       const std::string& rPath)
{
    const double ScalingFactor = ToNumericValue(
        FindKeyInJson(rEvent, "scaling_factor", rPath), rPath + ".scaling_factor");
    if (ScalingFactor <= 1.0)
    {
        ThrowConfigError(rPath + ".scaling_factor", "must be greater than 1");
    }
    return VoltageSwellEvent{StartTimeNs, DurationNs,
                             ParseEventTargetChannels(rEvent, rChannelIds, rPath), ScalingFactor};
}


FrequencyDistortionEvent ConfigParser::ParseFrequencyDistortionEvent(const Json& rEvent,
                                                                     std::uint64_t StartTimeNs,
                                                                     std::uint64_t DurationNs,
                                                                     double FrequencyHz,
                                                                     const std::string& rPath)
{
    FrequencyDistortionMode Mode;

    const std::string ModeString = ToStringValue(FindKeyInJson(rEvent, "mode", rPath), rPath + ".mode");

    if (ModeString == "step")
    {
        Mode = FrequencyDistortionMode::Step;
    }
    else if (ModeString == "ramp")
    {
        Mode = FrequencyDistortionMode::Ramp;
    }
    else
    {
        ThrowConfigError(rPath + ".mode", "must be 'step' or 'ramp'");
    }

    const double DeltaFrequencyHz = ToNumericValue(FindKeyInJson(rEvent, "delta_frequency_hz", rPath), rPath + ".delta_frequency_hz");

    if (!std::isfinite(FrequencyHz + DeltaFrequencyHz) || FrequencyHz + DeltaFrequencyHz <= 0.0)
    {
        ThrowConfigError(rPath + ".delta_frequency_hz", "would make the signal frequency non-positive");
    }

    return FrequencyDistortionEvent{Mode, StartTimeNs, DurationNs, DeltaFrequencyHz};
}


void ConfigParser::ValidateSchemaReference(const Json& rRoot)
{
    if (rRoot.contains("$schema") &&
        ToStringValue(rRoot.at("$schema"), "$.$schema") != "./signal_simulator.schema.json")
    {
        ThrowConfigError("$.$schema", "must be './signal_simulator.schema.json'");
    }
}


void ConfigParser::ParseSignalConfiguration(const Json& rRoot,
                                            SimulatorConfig& rConfig,
                                            std::set<std::string>& rChannelIds)
{
    const Json& Signal = FindKeyInJson(rRoot, "signal", "$");
    RequireJsonFieldEquals(Signal, "waveform", "sine", "signal");
    rConfig.FrequencyHz = ToNumericValue(FindKeyInJson(Signal, "frequency_hz", "signal"), "signal.frequency_hz");

    if (rConfig.FrequencyHz <= 0.0)
    {
        ThrowConfigError("signal.frequency_hz", "must be greater than zero");
    }

    rConfig.NominalAmplitudeV = ToNumericValue(FindKeyInJson(Signal, "nominal_amplitude_v", "signal"), "signal.nominal_amplitude_v");
    if (rConfig.NominalAmplitudeV < 0.0)
    {
        ThrowConfigError("signal.nominal_amplitude_v", "must not be negative");
    }

    rConfig.OffsetV = ToNumericValue(FindKeyInJson(Signal, "offset_v", "signal"), "signal.offset_v");

    ParseChannelConfigurations(Signal, rConfig, rChannelIds);
}


void ConfigParser::ParseChannelConfigurations(const Json& rSignal,
                                              SimulatorConfig& rConfig,
                                              std::set<std::string>& rChannelIds)
{
    const Json& Channels = FindKeyInJson(rSignal, "channels", "signal");

    if (!Channels.is_array() || Channels.empty())
    {
        ThrowConfigError("signal.channels", "must contain at least one channel");
    }

    for (std::size_t Index = 0; Index < Channels.size(); ++Index)
    {
        const Json& Channel = Channels[Index];
        const std::string Path = "signal.channels[" + std::to_string(Index) + "]";
        const std::string Id = ToStringValue(FindKeyInJson(Channel, "id", Path), Path + ".id");
        if (Id.empty())
        {
            ThrowConfigError(Path + ".id", "must not be empty");
        }

        if (!rChannelIds.insert(Id).second)
        {
            ThrowConfigError(Path + ".id", "channel IDs must be unique");
        }

        const double PhaseDegrees = ToNumericValue(FindKeyInJson(Channel, "phase_deg", Path), Path + ".phase_deg");
        rConfig.Channels.push_back(ChannelConfig{Id, PhaseDegrees});
    }
}


void ConfigParser::ParseSamplingConfiguration(const Json& rRoot, SimulatorConfig& rConfig)
{
    const Json& Sampling = FindKeyInJson(rRoot, "sampling", "$");

    const std::uint64_t SamplesPerCycle = ToUnsignedIntegerValue(
        FindKeyInJson(Sampling, "samples_per_cycle", "sampling"),
        "sampling.samples_per_cycle");

    if (SamplesPerCycle == 0)
    {
        ThrowConfigError("sampling.samples_per_cycle", "must be greater than zero");
    }

    rConfig.SamplesPerCycle = SamplesPerCycle;
    rConfig.SampleRateHz = rConfig.FrequencyHz * static_cast<double>(SamplesPerCycle);

    if (rConfig.SampleRateHz <= 0.0)
    {
        ThrowConfigError("sampling.samples_per_cycle", "derived sample rate must be greater than zero");
    }
}


void ConfigParser::ParseOutputConfiguration(const Json& rRoot, SimulatorConfig& rConfig)
{
    const Json& Output = FindKeyInJson(rRoot, "output", "$");
    ParseOutputRepresentationAndLayout(Output, rConfig);

    if (rConfig.Representation == SampleRepresentation::AdcInteger)
    {
        ParseAdcConfiguration(Output, rConfig);
    }
    else if (Output.contains("adc"))
    {
        ThrowConfigError("output.adc", "must not be present for floating-point output");
    }
}


void ConfigParser::ParseOutputRepresentationAndLayout(const Json& rOutput,
                                                      SimulatorConfig& rConfig)
{
    const std::string Representation =
        ToStringValue(FindKeyInJson(rOutput, "representation", "output"), "output.representation");
    if (Representation == "float32")
    {
        rConfig.Representation = SampleRepresentation::Float32;
    }
    else if (Representation == "adc_integer")
    {
        rConfig.Representation = SampleRepresentation::AdcInteger;
    }
    else
    {
        ThrowConfigError("output.representation", "must be 'float32' or 'adc_integer'");
    }
    const std::string Layout =
        ToStringValue(FindKeyInJson(rOutput, "layout", "output"), "output.layout");
    if (Layout == "interleaved")
    {
        rConfig.Layout = ChannelDataLayout::Interleaved;
    }
    else if (Layout == "planar")
    {
        rConfig.Layout = ChannelDataLayout::Planar;
    }
    else
    {
        ThrowConfigError("output.layout", "must be 'interleaved' or 'planar'");
    }
}


void ConfigParser::ParseAdcConfiguration(const Json& rOutput, SimulatorConfig& rConfig)
{
    const Json& Adc = FindKeyInJson(rOutput, "adc", "output");
    AdcConfig AdcSettings;
    AdcSettings.ReferenceVoltageV = ToNumericValue(
        FindKeyInJson(Adc, "reference_voltage_v", "output.adc"),
        "output.adc.reference_voltage_v");
    if (AdcSettings.ReferenceVoltageV <= 0.0)
    {
        ThrowConfigError("output.adc.reference_voltage_v", "must be greater than zero");
    }
    const std::string Polarity =
        ToStringValue(FindKeyInJson(Adc, "polarity", "output.adc"), "output.adc.polarity");
    if (Polarity == "unipolar")
    {
        AdcSettings.Polarity = AdcPolarity::Unipolar;
    }
    else if (Polarity == "bipolar")
    {
        AdcSettings.Polarity = AdcPolarity::Bipolar;
    }
    else
    {
        ThrowConfigError("output.adc.polarity", "must be 'unipolar' or 'bipolar'");
    }
    const std::uint64_t ResolutionBits = ToUnsignedIntegerValue(
        FindKeyInJson(Adc, "resolution_bits", "output.adc"), "output.adc.resolution_bits");
    if (ResolutionBits != 8 && ResolutionBits != 12 && ResolutionBits != 16 &&
        ResolutionBits != 24)
    {
        ThrowConfigError("output.adc.resolution_bits", "must be 8, 12, 16, or 24");
    }
    AdcSettings.ResolutionBits = static_cast<std::uint32_t>(ResolutionBits);
    rConfig.Adc = AdcSettings;
}


void ConfigParser::ParseBufferConfiguration(const Json& rRoot, SimulatorConfig& rConfig)
{
    const Json& Buffer = FindKeyInJson(rRoot, "buffer", "$");
    rConfig.SlotCount = ToSizeTValue(FindKeyInJson(Buffer, "slot_count", "buffer"),
                                     "buffer.slot_count");
    if (rConfig.SlotCount < 2)
    {
        ThrowConfigError("buffer.slot_count", "must be at least 2");
    }
    rConfig.BlockSizeSamplesPerChannel = ToSizeTValue(
        FindKeyInJson(Buffer, "block_size_samples_per_channel", "buffer"),
        "buffer.block_size_samples_per_channel");
    if (rConfig.BlockSizeSamplesPerChannel == 0)
    {
        ThrowConfigError("buffer.block_size_samples_per_channel", "must be greater than zero");
    }

    ValidateSamplePayloadSize(rConfig);
}


void ConfigParser::ValidateSamplePayloadSize(const SimulatorConfig& rConfig)
{
    const std::size_t BytesPerSample = !rConfig.Adc.has_value() ? 4 : (rConfig.Adc->ResolutionBits == 8 ? 1 : (rConfig.Adc->ResolutionBits <= 16 ? 2 : 4));
    const std::size_t MaxSize = std::numeric_limits<std::size_t>::max();
    const std::size_t ChannelCount = rConfig.Channels.size();
    if (ChannelCount == 0 || rConfig.BlockSizeSamplesPerChannel > MaxSize / ChannelCount ||
        rConfig.BlockSizeSamplesPerChannel * ChannelCount > MaxSize / BytesPerSample)
    {
        ThrowConfigError("buffer.block_size_samples_per_channel", "sample payload size overflows this platform");
    }
}


void ConfigParser::ParseIpcConfiguration(const Json& rRoot, SimulatorConfig& rConfig)
{
    const Json& Ipc = FindKeyInJson(rRoot, "ipc", "$");
    rConfig.SharedMemoryName = ToStringValue(
        FindKeyInJson(Ipc, "shared_memory_name", "ipc"), "ipc.shared_memory_name");
    ValidatePosixIpcName(rConfig.SharedMemoryName, "ipc.shared_memory_name");
    ParseNotificationConfiguration(Ipc, rConfig);
}


void ConfigParser::ParseNotificationConfiguration(const Json& rIpc, SimulatorConfig& rConfig)
{
    const Json& Notification = FindKeyInJson(rIpc, "notification", "ipc");
    rConfig.NotificationType = ToStringValue(
        FindKeyInJson(Notification, "type", "ipc.notification"), "ipc.notification.type");
    if (rConfig.NotificationType != "posix_named_semaphore")
    {
        ThrowConfigError("ipc.notification.type", "must be 'posix_named_semaphore'");
    }
    rConfig.NotificationName = ToStringValue(
        FindKeyInJson(Notification, "name", "ipc.notification"), "ipc.notification.name");
    ValidatePosixIpcName(rConfig.NotificationName, "ipc.notification.name");
    const std::uint64_t InitialValue = ToUnsignedIntegerValue(
        FindKeyInJson(Notification, "initial_value", "ipc.notification"),
        "ipc.notification.initial_value");
    if (InitialValue != 0)
    {
        ThrowConfigError("ipc.notification.initial_value", "must be 0");
    }
}


void ConfigParser::ValidateNominalSignalWithinAdcRange(const SimulatorConfig& rConfig)
{
    if (!rConfig.Adc.has_value())
    {
        return;
    }

    const double LowerVoltage = rConfig.OffsetV - rConfig.NominalAmplitudeV;
    const double UpperVoltage = rConfig.OffsetV + rConfig.NominalAmplitudeV;
    if (rConfig.Adc->Polarity == AdcPolarity::Bipolar)
    {
        const double PeakEnvelope = std::abs(rConfig.OffsetV) + rConfig.NominalAmplitudeV;
        if (!std::isfinite(PeakEnvelope) || PeakEnvelope > rConfig.Adc->ReferenceVoltageV)
        {
            ThrowConfigError("signal", "nominal waveform exceeds the bipolar ADC range");
        }
    }
    else if (!std::isfinite(LowerVoltage) || !std::isfinite(UpperVoltage) ||
             LowerVoltage < 0.0 || UpperVoltage > rConfig.Adc->ReferenceVoltageV)
    {
        ThrowConfigError("signal", "nominal waveform exceeds the unipolar ADC range");
    }
}


void ConfigParser::ParseDisturbanceTimeline(const Json& rRoot, SimulatorConfig& rConfig,
                                            const std::set<std::string>& rChannelIds)
{
    if (!rRoot.contains("disturbance_timeline"))
    {
        return;
    }
    const Json& Timeline = rRoot.at("disturbance_timeline");
    if (!Timeline.is_array())
    {
        ThrowConfigError("disturbance_timeline", "expected an array");
    }
    for (std::size_t Index = 0; Index < Timeline.size(); ++Index)
    {
        rConfig.DisturbanceTimeline.push_back(
            ParseDisturbanceEvent(Timeline[Index], Index, rChannelIds, rConfig.FrequencyHz));
    }
}


SimulatorConfig ConfigParser::ParseConfigDocument(const Json& rRoot)
{
    ValidateSchemaReference(rRoot);

    SimulatorConfig Config;
    std::set<std::string> ChannelIds;
    ParseSignalConfiguration(rRoot, Config, ChannelIds);
    ParseSamplingConfiguration(rRoot, Config);
    ParseOutputConfiguration(rRoot, Config);
    ParseBufferConfiguration(rRoot, Config);
    ParseIpcConfiguration(rRoot, Config);
    ValidateNominalSignalWithinAdcRange(Config);
    ParseDisturbanceTimeline(rRoot, Config, ChannelIds);
    return Config;
}


SimulatorConfig ConfigParser::ValidateAndConvertJsonDocument(const Json& rDocument)
{
    try
    {
        return ParseConfigDocument(rDocument);
    }
    catch (const Json::exception& Error)
    {
        throw std::invalid_argument(std::string("invalid configuration: ") + Error.what());
    }
}


SimulatorConfig ConfigParser::ParseConfigFile(const std::string& rPath)
{
    std::ifstream Input(rPath);
    if (!Input)
    {
        throw std::runtime_error("cannot open configuration file '" + rPath + "'");
    }
    try
    {
        return ValidateAndConvertJsonDocument(Json::parse(Input));
    }
    catch (const Json::parse_error& Error)
    {
        throw std::invalid_argument(std::string("invalid JSON in '") + rPath + "': " + Error.what());
    }
}