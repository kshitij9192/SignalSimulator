#pragma once

#include <cstddef>
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
    std::size_t Id = 0;
    double PhaseDegrees = 0.0;
};


struct AdcConfig
{
    double ReferenceAmplitude = 0.0;
    AdcPolarity Polarity = AdcPolarity::Bipolar;
    std::uint32_t ResolutionBits = 0;

    std::size_t GetBytesPerSample() const
    {
        std::size_t BytesPerSample = 0;

        switch (ResolutionBits)
        {
        case 8:
            BytesPerSample = 1;
            break;
        case 12:
        case 16:
            BytesPerSample = 2;
            break;
        case 24:
        default:
            BytesPerSample = 4;
            break;
        }

        return BytesPerSample;
    }
};


struct SurgeEvent
{
    std::uint64_t StartTimeNs = 0;
    std::uint64_t DurationNs = 0;
    std::vector<std::size_t> TargetChannels;
    double PeakAmplitude = 0.0;
};


struct SagEvent
{
    std::uint64_t StartTimeNs = 0;
    std::uint64_t DurationNs = 0;
    std::vector<std::size_t> TargetChannels;
    double ScalingFactor = 0.0;
};


struct SwellEvent
{
    std::uint64_t StartTimeNs = 0;
    std::uint64_t DurationNs = 0;
    std::vector<std::size_t> TargetChannels;
    double ScalingFactor = 0.0;
};


struct FrequencyDistortionEvent
{
    FrequencyDistortionMode Mode = FrequencyDistortionMode::Step;
    std::uint64_t StartTimeNs = 0;
    std::uint64_t DurationNs = 0;
    double DeltaFrequencyHz = 0.0;
};


using DisturbanceEvent = std::variant<SurgeEvent, SagEvent, SwellEvent, FrequencyDistortionEvent>;

struct SimulatorConfig
{
    double FrequencyHz = 0.0;
    double NominalAmplitude = 0.0;
    double Offset = 0.0;
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

    std::size_t GetBytesPerSample() const
    {
        std::size_t BytesPerSample = sizeof(float);

        if (Representation != SampleRepresentation::Float32 && Adc.has_value())
        {
            BytesPerSample = Adc->GetBytesPerSample();
        }

        return BytesPerSample;
    }
};
