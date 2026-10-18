#include "AdcSampleQuantizer.h"

#include <algorithm>
#include <cmath>
#include <cstring>

AdcSampleQuantizer::AdcSampleQuantizer(const AdcConfig& rAdcConfig)
    : m_rAdcConfig(rAdcConfig), m_BytesPerSample(rAdcConfig.GetBytesPerSample())
{
}


std::uint32_t AdcSampleQuantizer::QuantizeUnipolarCode(double InputSample) const
{
    const double ClampedInputValue = std::clamp(InputSample, 0.0, m_rAdcConfig.ReferenceAmplitude);
    const std::uint64_t MaxADCValue = (1ULL << m_rAdcConfig.ResolutionBits) - 1ULL;
    const double NormalizedInputValue = ClampedInputValue / m_rAdcConfig.ReferenceAmplitude;
    const std::int64_t RoundedADCValue = std::llround(NormalizedInputValue * static_cast<double>(MaxADCValue));

    return static_cast<std::uint32_t>(RoundedADCValue);
}


std::int32_t AdcSampleQuantizer::QuantizeBipolarCode(double InputSample) const
{
    const double ReferenceAmplitude = m_rAdcConfig.ReferenceAmplitude;
    const double ClampedInputValue = std::clamp(InputSample, -ReferenceAmplitude, ReferenceAmplitude);
    const std::int64_t HalfScale = 1LL << (m_rAdcConfig.ResolutionBits - 1U);
    const double NormalizedInputValue = (ClampedInputValue / ReferenceAmplitude) * static_cast<double>(HalfScale);
    const std::int64_t RoundedADCValue = std::llround(NormalizedInputValue);

    return static_cast<std::int32_t>(RoundedADCValue);
}


void AdcSampleQuantizer::WriteUnipolarADCValue(std::uint32_t InputADCValue,
                                               std::byte* pDestination) const
{
    if (m_BytesPerSample == 1)
    {
        const auto Value = static_cast<std::uint8_t>(InputADCValue);
        std::memcpy(pDestination, &Value, sizeof(Value));
    }
    else if (m_BytesPerSample == 2)
    {
        const auto Value = static_cast<std::uint16_t>(InputADCValue);
        std::memcpy(pDestination, &Value, sizeof(Value));
    }
    else
    {
        std::memcpy(pDestination, &InputADCValue, sizeof(InputADCValue));
    }
}


void AdcSampleQuantizer::WriteBipolarADCValue(std::int32_t InputADCValue,
                                              std::byte* pDestination) const
{
    if (m_BytesPerSample == 1)
    {
        const auto Value = static_cast<std::int8_t>(InputADCValue);
        std::memcpy(pDestination, &Value, sizeof(Value));
    }
    else if (m_BytesPerSample == 2)
    {
        const auto Value = static_cast<std::int16_t>(InputADCValue);
        std::memcpy(pDestination, &Value, sizeof(Value));
    }
    else
    {
        std::memcpy(pDestination, &InputADCValue, sizeof(InputADCValue));
    }
}


void AdcSampleQuantizer::QuantizeSampleAndCopyToMemory(double SampleValue,
                                                       std::byte* pDestination) const
{
    if (m_rAdcConfig.Polarity == AdcPolarity::Unipolar)
    {
        WriteUnipolarADCValue(QuantizeUnipolarCode(SampleValue), pDestination);
    }
    else
    {
        WriteBipolarADCValue(QuantizeBipolarCode(SampleValue), pDestination);
    }
}


std::size_t AdcSampleQuantizer::GetBytesPerSample() const
{
    return m_BytesPerSample;
}
