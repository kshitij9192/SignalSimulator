#include "output/AdcSampleQuantizer.h"

#include <algorithm>
#include <cmath>
#include <cstring>

AdcSampleQuantizer::AdcSampleQuantizer(const AdcConfig& AdcConfigValue)
    : m_AdcConfig(AdcConfigValue), m_BytesPerSample(AdcConfigValue.GetBytesPerSample())
{
}


std::uint32_t AdcSampleQuantizer::QuantizeUnipolarCode(double InputSample) const
{
    const double ClampedInputValue = std::clamp(InputSample, 0.0, m_AdcConfig.ReferenceAmplitude);
    const std::uint64_t MaxADCValue = (1ULL << m_AdcConfig.ResolutionBits) - 1ULL;
    const double NormalizedInputValue = ClampedInputValue / m_AdcConfig.ReferenceAmplitude;
    const std::int64_t RoundedADCValue = std::llround(NormalizedInputValue * static_cast<double>(MaxADCValue));

    return static_cast<std::uint32_t>(RoundedADCValue);
}


std::int32_t AdcSampleQuantizer::QuantizeBipolarCode(double InputSample) const
{
    const double ReferenceAmplitude = m_AdcConfig.ReferenceAmplitude;
    const double ClampedInputValue = std::clamp(InputSample, -ReferenceAmplitude, ReferenceAmplitude);
    const auto HalfScale = static_cast<std::int64_t>(1ULL << (m_AdcConfig.ResolutionBits - 1U));
    const std::int64_t MinADCValue = -HalfScale;
    const std::int64_t MaxADCValue = HalfScale - 1;
    const double NormalizedInputValue =
        (ClampedInputValue / ReferenceAmplitude) * static_cast<double>(HalfScale);
    const std::int64_t RoundedADCValue = std::llround(NormalizedInputValue);
    const std::int64_t ClampedADCValue = std::clamp(RoundedADCValue, MinADCValue, MaxADCValue);

    return static_cast<std::int32_t>(ClampedADCValue);
}


void AdcSampleQuantizer::WriteUnipolarADCValue(std::uint32_t InputADCValue,
                                               std::byte* Destination) const
{
    if (m_BytesPerSample == 1)
    {
        const auto Value = static_cast<std::uint8_t>(InputADCValue);
        std::memcpy(Destination, &Value, sizeof(Value));
    }
    else if (m_BytesPerSample == 2)
    {
        const auto Value = static_cast<std::uint16_t>(InputADCValue);
        std::memcpy(Destination, &Value, sizeof(Value));
    }
    else
    {
        std::memcpy(Destination, &InputADCValue, sizeof(InputADCValue));
    }
}


void AdcSampleQuantizer::WriteBipolarADCValue(std::int32_t InputADCValue,
                                              std::byte* Destination) const
{
    if (m_BytesPerSample == 1)
    {
        const auto Value = static_cast<std::int8_t>(InputADCValue);
        std::memcpy(Destination, &Value, sizeof(Value));
    }
    else if (m_BytesPerSample == 2)
    {
        const auto Value = static_cast<std::int16_t>(InputADCValue);
        std::memcpy(Destination, &Value, sizeof(Value));
    }
    else
    {
        std::memcpy(Destination, &InputADCValue, sizeof(InputADCValue));
    }
}


void AdcSampleQuantizer::QuantizeSampleAndCopyToMemory(double SampleValue,
                                                       std::byte* Destination) const
{
    if (m_AdcConfig.Polarity == AdcPolarity::Unipolar)
    {
        WriteUnipolarADCValue(QuantizeUnipolarCode(SampleValue), Destination);
    }
    else
    {
        WriteBipolarADCValue(QuantizeBipolarCode(SampleValue), Destination);
    }
}
