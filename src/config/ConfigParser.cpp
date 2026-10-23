#include "config/ConfigParser.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <fstream>
#include <limits>
#include <set>
#include <stdexcept>

[[noreturn]] void ConfigParser::ThrowConfigError(const std::string& Path, const std::string& Message)
{
    throw std::invalid_argument(Path + ": " + Message);
}


const ConfigParser::Json& ConfigParser::FindKeyInJson(const Json& Object,
                                                      std::string_view Key,
                                                      std::string_view Path)
{
    const auto It = Object.find(Key);
    if (It == Object.end())
    {
        ThrowConfigError(std::string(Path) + "." + std::string(Key),
                         "required property is missing");
    }

    return *It;
}


std::string ConfigParser::ToStringValue(const Json& Value, const std::string& Path)
{
    if (!Value.is_string())
    {
        ThrowConfigError(Path, "expected a string");
    }

    return Value.get<std::string>();
}


double ConfigParser::ToNumericValue(const Json& Value, const std::string& Path)
{
    if (!Value.is_number())
    {
        ThrowConfigError(Path, "expected a number");
    }

    return Value.get<double>();
}


std::uint64_t ConfigParser::ToUnsignedIntegerValue(const Json& Value, const std::string& Path)
{
    if (Value.is_number_unsigned())
    {
        return Value.get<std::uint64_t>();
    }

    ThrowConfigError(Path, "expected a non-negative integer");
}


std::size_t ConfigParser::ToSizeTValue(const Json& Value, const std::string& Path)
{
    const std::uint64_t UnsignedValue = ToUnsignedIntegerValue(Value, Path);

    return static_cast<std::size_t>(UnsignedValue);
}


void ConfigParser::RequireJsonFieldEquals(const Json& Object, const std::string& Key,
                                          const Json& Expected, const std::string& Path)
{
    const Json& Value = FindKeyInJson(Object, Key, Path);
    if (Value != Expected)
    {
        ThrowConfigError(Path + "." + Key, "has an unsupported value");
    }
}


void ConfigParser::ValidatePosixIpcName(const std::string& Name, const std::string& Path)
{
    if (Name.size() < 2 || Name.front() != '/' || Name.find('/', 1) != std::string::npos ||
        Name.find('\0') != std::string::npos)
    {
        ThrowConfigError(Path, "must begin with '/' and contain no other '/' characters");
    }
}


std::uint64_t ConfigParser::ParseEventStartTime(const Json& Event, const std::string& Path)
{
    return ToUnsignedIntegerValue(FindKeyInJson(Event, "start_time_ns", Path),
                                  Path + ".start_time_ns");
}


std::uint64_t ConfigParser::ParseEventDurationNanoseconds(const Json& Event, std::uint64_t StartTimeNs,
                                                          const std::string& Path)
{
    const std::uint64_t DurationNs =
        ToUnsignedIntegerValue(FindKeyInJson(Event, "duration_ns", Path), Path + ".duration_ns");
    if (DurationNs == 0)
    {
        ThrowConfigError(Path + ".duration_ns", "must be greater than zero");
    }
    if (DurationNs > (std::numeric_limits<std::uint64_t>::max() - StartTimeNs))
    {
        ThrowConfigError(Path, "start_time_ns plus duration_ns overflows");
    }

    return DurationNs;
}


std::vector<std::size_t> ConfigParser::ParseEventTargetChannels(const Json& Event,
                                                                std::size_t ChannelCount,
                                                                const std::string& Path)
{
    const Json& Targets = FindKeyInJson(Event, "target_channels", Path);
    if (!Targets.is_array() || Targets.empty())
    {
        ThrowConfigError(Path + ".target_channels", "expected a non-empty array");
    }

    std::set<std::size_t> UniqueTargets;
    std::vector<std::size_t> Result;
    for (std::size_t Index = 0; Index < Targets.size(); ++Index)
    {
        const std::size_t TargetChannel =
            ToSizeTValue(Targets[Index], Path + ".target_channels[" + std::to_string(Index) + "]");
        if (!UniqueTargets.insert(TargetChannel).second)
        {
            ThrowConfigError(Path + ".target_channels", "channel indices must be unique");
        }
        if (TargetChannel >= ChannelCount)
        {
            ThrowConfigError(Path + ".target_channels",
                             "references out-of-range channel index " +
                                 std::to_string(TargetChannel));
        }
        Result.push_back(TargetChannel);
    }
    return Result;
}


DisturbanceEvent ConfigParser::ParseDisturbanceEvent(const Json& Event, std::size_t Index,
                                                     std::size_t ChannelCount, double FrequencyHz)
{
    const std::string Path = "disturbance_timeline[" + std::to_string(Index) + "]";
    const std::string Type = ToStringValue(FindKeyInJson(Event, "type", Path), Path + ".type");
    const std::uint64_t StartTimeNs = ParseEventStartTime(Event, Path);
    const std::uint64_t DurationNs = ParseEventDurationNanoseconds(Event, StartTimeNs, Path);

    if (Type == "surge")
    {
        return ParseSurgeEvent(Event, StartTimeNs, DurationNs, ChannelCount, Path);
    }

    if (Type == "sag")
    {
        return ParseSagEvent(Event, StartTimeNs, DurationNs, ChannelCount, Path);
    }

    if (Type == "swell")
    {
        return ParseSwellEvent(Event, StartTimeNs, DurationNs, ChannelCount, Path);
    }

    if (Type == "frequency_distortion")
    {
        return ParseFrequencyDistortionEvent(Event, StartTimeNs, DurationNs, FrequencyHz, Path);
    }

    ThrowConfigError(Path + ".type", "must be 'surge', 'sag', 'swell', or 'frequency_distortion'");
}


SurgeEvent ConfigParser::ParseSurgeEvent(const Json& Event, std::uint64_t StartTimeNs,
                                         std::uint64_t DurationNs, std::size_t ChannelCount,
                                         const std::string& Path)
{
    RequireJsonFieldEquals(Event, "shape", "half_sine", Path);
    const double PeakAmplitude = ToNumericValue(
        FindKeyInJson(Event, "peak_amplitude", Path), Path + ".peak_amplitude");
    if (!std::isfinite(PeakAmplitude) || PeakAmplitude == 0.0)
    {
        ThrowConfigError(Path + ".peak_amplitude", "must be non-zero");
    }
    return SurgeEvent{StartTimeNs, DurationNs,
                      ParseEventTargetChannels(Event, ChannelCount, Path), PeakAmplitude};
}


SagEvent ConfigParser::ParseSagEvent(const Json& Event,
                                     std::uint64_t StartTimeNs,
                                     std::uint64_t DurationNs,
                                     std::size_t ChannelCount,
                                     const std::string& Path)
{
    const double ScalingFactor = ToNumericValue(
        FindKeyInJson(Event, "scaling_factor", Path), Path + ".scaling_factor");
    if (ScalingFactor < 0.0 || ScalingFactor >= 1.0)
    {
        ThrowConfigError(Path + ".scaling_factor",
                         "must be greater than or equal to 0 and less than 1");
    }
    return SagEvent{StartTimeNs, DurationNs,
                    ParseEventTargetChannels(Event, ChannelCount, Path), ScalingFactor};
}


SwellEvent ConfigParser::ParseSwellEvent(const Json& Event,
                                         std::uint64_t StartTimeNs,
                                         std::uint64_t DurationNs,
                                         std::size_t ChannelCount,
                                         const std::string& Path)
{
    const double ScalingFactor = ToNumericValue(
        FindKeyInJson(Event, "scaling_factor", Path), Path + ".scaling_factor");
    if (ScalingFactor <= 1.0)
    {
        ThrowConfigError(Path + ".scaling_factor", "must be greater than 1");
    }
    return SwellEvent{StartTimeNs, DurationNs,
                      ParseEventTargetChannels(Event, ChannelCount, Path), ScalingFactor};
}


FrequencyDistortionEvent ConfigParser::ParseFrequencyDistortionEvent(const Json& Event,
                                                                     std::uint64_t StartTimeNs,
                                                                     std::uint64_t DurationNs,
                                                                     double FrequencyHz,
                                                                     const std::string& Path)
{
    FrequencyDistortionMode Mode;

    const std::string ModeString = ToStringValue(FindKeyInJson(Event, "mode", Path), Path + ".mode");

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
        ThrowConfigError(Path + ".mode", "must be 'step' or 'ramp'");
    }

    const double DeltaFrequencyHz = ToNumericValue(FindKeyInJson(Event, "delta_frequency_hz", Path), Path + ".delta_frequency_hz");

    if (!std::isfinite(FrequencyHz + DeltaFrequencyHz) || FrequencyHz + DeltaFrequencyHz <= 0.0)
    {
        ThrowConfigError(Path + ".delta_frequency_hz", "would make the signal frequency non-positive");
    }

    return FrequencyDistortionEvent{Mode, StartTimeNs, DurationNs, DeltaFrequencyHz};
}


void ConfigParser::ValidateSchemaReference(const Json& Root)
{
    if (Root.contains("$schema") &&
        ToStringValue(Root.at("$schema"), "$.$schema") != "./signal_simulator.schema.json")
    {
        ThrowConfigError("$.$schema", "must be './signal_simulator.schema.json'");
    }
}


void ConfigParser::ParseSignalConfiguration(const Json& Root, SimulatorConfig& Config)
{
    const Json& Signal = FindKeyInJson(Root, "signal", "$");
    RequireJsonFieldEquals(Signal, "waveform", "sine", "signal");
    Config.FrequencyHz = ToNumericValue(FindKeyInJson(Signal, "frequency_hz", "signal"), "signal.frequency_hz");

    if (Config.FrequencyHz <= 0.0)
    {
        ThrowConfigError("signal.frequency_hz", "must be greater than zero");
    }

    Config.NominalAmplitude = ToNumericValue(FindKeyInJson(Signal, "nominal_amplitude", "signal"), "signal.nominal_amplitude");
    if (Config.NominalAmplitude < 0.0)
    {
        ThrowConfigError("signal.nominal_amplitude", "must not be negative");
    }

    Config.Offset = ToNumericValue(FindKeyInJson(Signal, "offset", "signal"), "signal.offset");

    ParseChannelConfigurations(Signal, Config);
}


void ConfigParser::ParseChannelConfigurations(const Json& Signal, SimulatorConfig& Config)
{
    const Json& Channels = FindKeyInJson(Signal, "channels", "signal");

    if (!Channels.is_array() || Channels.empty())
    {
        ThrowConfigError("signal.channels", "must contain at least one channel");
    }

    for (std::size_t Index = 0; Index < Channels.size(); ++Index)
    {
        const Json& Channel = Channels[Index];
        const std::string Path = "signal.channels[" + std::to_string(Index) + "]";
        const double PhaseDegrees =
            ToNumericValue(FindKeyInJson(Channel, "phase_deg", Path), Path + ".phase_deg");
        Config.Channels.push_back(ChannelConfig{Index, PhaseDegrees});
    }
}


void ConfigParser::ParseSamplingConfiguration(const Json& Root, SimulatorConfig& Config)
{
    const Json& Sampling = FindKeyInJson(Root, "sampling", "$");

    const std::uint64_t SamplesPerCycle = ToUnsignedIntegerValue(
        FindKeyInJson(Sampling, "samples_per_cycle", "sampling"),
        "sampling.samples_per_cycle");

    if (SamplesPerCycle == 0)
    {
        ThrowConfigError("sampling.samples_per_cycle", "must be greater than zero");
    }

    Config.SamplesPerCycle = SamplesPerCycle;
    Config.SampleRateHz = Config.FrequencyHz * static_cast<double>(SamplesPerCycle);

    if (Config.SampleRateHz <= 0.0)
    {
        ThrowConfigError("sampling.samples_per_cycle", "derived sample rate must be greater than zero");
    }
}


void ConfigParser::ParseOutputConfiguration(const Json& Root, SimulatorConfig& Config)
{
    const Json& Output = FindKeyInJson(Root, "output", "$");
    ParseOutputRepresentationAndLayout(Output, Config);

    if (Config.Representation == SampleRepresentation::AdcInteger)
    {
        ParseAdcConfiguration(Output, Config);
    }
    else if (Output.contains("adc"))
    {
        ThrowConfigError("output.adc", "must not be present for floating-point output");
    }
}


void ConfigParser::ParseOutputRepresentationAndLayout(const Json& Output,
                                                      SimulatorConfig& Config)
{
    const std::string Representation =
        ToStringValue(FindKeyInJson(Output, "representation", "output"), "output.representation");
    if (Representation == "float32")
    {
        Config.Representation = SampleRepresentation::Float32;
    }
    else if (Representation == "adc_integer")
    {
        Config.Representation = SampleRepresentation::AdcInteger;
    }
    else
    {
        ThrowConfigError("output.representation", "must be 'float32' or 'adc_integer'");
    }
    const std::string Layout =
        ToStringValue(FindKeyInJson(Output, "layout", "output"), "output.layout");
    if (Layout == "interleaved")
    {
        Config.Layout = ChannelDataLayout::Interleaved;
    }
    else if (Layout == "planar")
    {
        Config.Layout = ChannelDataLayout::Planar;
    }
    else
    {
        ThrowConfigError("output.layout", "must be 'interleaved' or 'planar'");
    }
}


void ConfigParser::ParseAdcConfiguration(const Json& Output, SimulatorConfig& Config)
{
    const Json& Adc = FindKeyInJson(Output, "adc", "output");
    AdcConfig AdcSettings;
    AdcSettings.ReferenceAmplitude = ToNumericValue(
        FindKeyInJson(Adc, "reference_amplitude", "output.adc"),
        "output.adc.reference_amplitude");
    if (AdcSettings.ReferenceAmplitude <= 0.0)
    {
        ThrowConfigError("output.adc.reference_amplitude", "must be greater than zero");
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
    Config.Adc = AdcSettings;
}


void ConfigParser::ParseBufferConfiguration(const Json& Root, SimulatorConfig& Config)
{
    const Json& Buffer = FindKeyInJson(Root, "buffer", "$");
    Config.SlotCount = ToSizeTValue(FindKeyInJson(Buffer, "slot_count", "buffer"),
                                    "buffer.slot_count");
    if (Config.SlotCount < 2 || Config.SlotCount > 6)
    {
        ThrowConfigError("buffer.slot_count", "must be between 2 and 6");
    }
    Config.BlockSizeSamplesPerChannel = ToSizeTValue(
        FindKeyInJson(Buffer, "block_size_samples_per_channel", "buffer"),
        "buffer.block_size_samples_per_channel");
    if (Config.BlockSizeSamplesPerChannel == 0)
    {
        ThrowConfigError("buffer.block_size_samples_per_channel", "must be greater than zero");
    }

    ValidateSamplePayloadSize(Config);
}


void ConfigParser::ValidateSamplePayloadSize(const SimulatorConfig& Config)
{
    const std::size_t BytesPerSample = Config.GetBytesPerSample();
    const std::size_t MaxSize = std::numeric_limits<std::size_t>::max();
    const std::size_t ChannelCount = Config.Channels.size();
    if (ChannelCount == 0 || Config.BlockSizeSamplesPerChannel > MaxSize / ChannelCount ||
        Config.BlockSizeSamplesPerChannel * ChannelCount > MaxSize / BytesPerSample)
    {
        ThrowConfigError("buffer.block_size_samples_per_channel", "sample payload size overflows this platform");
    }
}


void ConfigParser::ParseIpcConfiguration(const Json& Root, SimulatorConfig& Config)
{
    const Json& Ipc = FindKeyInJson(Root, "ipc", "$");
    Config.SharedMemoryName = ToStringValue(
        FindKeyInJson(Ipc, "shared_memory_name", "ipc"), "ipc.shared_memory_name");
    ValidatePosixIpcName(Config.SharedMemoryName, "ipc.shared_memory_name");
    Config.SemaphoreName =
        ToStringValue(FindKeyInJson(Ipc, "semaphore_name", "ipc"), "ipc.semaphore_name");
    ValidatePosixIpcName(Config.SemaphoreName, "ipc.semaphore_name");
}


void ConfigParser::ValidateNominalSignalWithinAdcRange(const SimulatorConfig& Config)
{
    if (!Config.Adc.has_value())
    {
        return;
    }

    const double LowerValue = Config.Offset - Config.NominalAmplitude;
    const double UpperValue = Config.Offset + Config.NominalAmplitude;
    if (Config.Adc->Polarity == AdcPolarity::Bipolar)
    {
        const double PeakEnvelope = std::abs(Config.Offset) + Config.NominalAmplitude;
        if (!std::isfinite(PeakEnvelope) || PeakEnvelope > Config.Adc->ReferenceAmplitude)
        {
            ThrowConfigError("signal", "nominal waveform exceeds the bipolar ADC range");
        }
    }
    else if (!std::isfinite(LowerValue) || !std::isfinite(UpperValue) ||
             LowerValue < 0.0 || UpperValue > Config.Adc->ReferenceAmplitude)
    {
        ThrowConfigError("signal", "nominal waveform exceeds the unipolar ADC range");
    }
}


void ConfigParser::ParseDisturbanceTimeline(const Json& Root, SimulatorConfig& Config)
{
    if (!Root.contains("disturbance_timeline"))
    {
        return;
    }
    const Json& Timeline = Root.at("disturbance_timeline");
    if (!Timeline.is_array())
    {
        ThrowConfigError("disturbance_timeline", "expected an array");
    }
    const std::size_t ChannelCount = Config.Channels.size();
    for (std::size_t Index = 0; Index < Timeline.size(); ++Index)
    {
        Config.DisturbanceTimeline.push_back(
            ParseDisturbanceEvent(Timeline[Index], Index, ChannelCount, Config.FrequencyHz));
    }
}


SimulatorConfig ConfigParser::ExtractAndValidateJson(const Json& Root)
{
    ValidateSchemaReference(Root);

    SimulatorConfig Config;
    ParseSignalConfiguration(Root, Config);
    ParseSamplingConfiguration(Root, Config);
    ParseOutputConfiguration(Root, Config);
    ParseBufferConfiguration(Root, Config);
    ParseIpcConfiguration(Root, Config);
    ValidateNominalSignalWithinAdcRange(Config);
    ParseDisturbanceTimeline(Root, Config);
    return Config;
}


SimulatorConfig ConfigParser::TryParsingJson(const Json& Document)
{
    try
    {
        return ExtractAndValidateJson(Document);
    }
    catch (const Json::exception& Error)
    {
        throw std::invalid_argument(std::string("invalid configuration: ") + Error.what());
    }
}


SimulatorConfig ConfigParser::ParseConfigFile(const std::string& Path)
{
    std::ifstream Input(Path);
    if (!Input)
    {
        throw std::runtime_error("cannot open configuration file '" + Path + "'");
    }
    try
    {
        return TryParsingJson(Json::parse(Input));
    }
    catch (const Json::parse_error& Error)
    {
        throw std::invalid_argument(std::string("invalid JSON in '") + Path + "': " + Error.what());
    }
}
