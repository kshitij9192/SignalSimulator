#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

enum class SampleRepresentation : std::uint32_t
{
    AdcInteger,
    Float32
};


enum class ChannelDataLayout : std::uint32_t
{
    Interleaved,
    Planar
};


enum class AdcPolarity : std::uint32_t
{
    Unipolar,
    Bipolar
};


enum class FrequencyDistortionMode
{
    Step,
    Ramp
};


struct ChannelConfig
{
    std::string Id;
    double PhaseDegrees = 0.0;
};


struct AdcConfig
{
    double ReferenceVoltageV = 0.0;
    AdcPolarity Polarity = AdcPolarity::Bipolar;
    std::uint32_t ResolutionBits = 0;
};


struct SurgeEvent
{
    std::uint64_t StartTimeNs = 0;
    std::uint64_t DurationNs = 0;
    std::vector<std::string> TargetChannels;
    double PeakAmplitudeV = 0.0;
};


struct VoltageSagEvent
{
    std::uint64_t StartTimeNs = 0;
    std::uint64_t DurationNs = 0;
    std::vector<std::string> TargetChannels;
    double ScalingFactor = 0.0;
};


struct VoltageSwellEvent
{
    std::uint64_t StartTimeNs = 0;
    std::uint64_t DurationNs = 0;
    std::vector<std::string> TargetChannels;
    double ScalingFactor = 0.0;
};


struct FrequencyDistortionEvent
{
    FrequencyDistortionMode Mode = FrequencyDistortionMode::Step;
    std::uint64_t StartTimeNs = 0;
    std::uint64_t DurationNs = 0;
    double DeltaFrequencyHz = 0.0;
};


using DisturbanceEvent = std::variant<SurgeEvent, VoltageSagEvent, VoltageSwellEvent, FrequencyDistortionEvent>;

struct SimulatorConfig
{
    double FrequencyHz = 0.0;
    double NominalAmplitudeV = 0.0;
    double OffsetV = 0.0;
    std::vector<ChannelConfig> Channels;
    std::optional<std::uint64_t> SamplesPerCycle;
    double SampleRateHz = 0.0;
    SampleRepresentation Representation = SampleRepresentation::Float32;
    ChannelDataLayout Layout = ChannelDataLayout::Interleaved;
    std::optional<AdcConfig> Adc;
    std::size_t SlotCount = 0;
    std::size_t BlockSizeSamplesPerChannel = 0;
    std::string SharedMemoryName;
    std::string NotificationType;
    std::string NotificationName;
    std::uint32_t NotificationInitialValue = 0;
    std::vector<DisturbanceEvent> DisturbanceTimeline;
};
