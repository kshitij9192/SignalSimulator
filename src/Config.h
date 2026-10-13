#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

enum class SampleRepresentation
{
    AdcInteger,
    Float32
};


enum class ChannelDataLayout
{
    Interleaved,
    Planar
};


enum class AdcPolarity
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


using DisturbanceEvent = std::variant<SurgeEvent, VoltageSagEvent, VoltageSwellEvent,
                                      FrequencyDistortionEvent>;
