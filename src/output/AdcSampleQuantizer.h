#pragma once

#include "config/Config.h"
#include "output/ISampleQuantizer.h"

#include <cstddef>
#include <cstdint>

class AdcSampleQuantizer final : public ISampleQuantizer
{
  public:
    explicit AdcSampleQuantizer(const AdcConfig& AdcConfigValue);
    ~AdcSampleQuantizer() override = default;

    AdcSampleQuantizer(const AdcSampleQuantizer&) = delete;
    AdcSampleQuantizer& operator=(const AdcSampleQuantizer&) = delete;

    void QuantizeSampleAndCopyToMemory(double InputSample, std::byte* Destination) const override;

  private:
    std::uint32_t QuantizeUnipolarCode(double InputSample) const;
    std::int32_t QuantizeBipolarCode(double InputSample) const;
    void WriteUnipolarADCValue(std::uint32_t Code, std::byte* Destination) const;
    void WriteBipolarADCValue(std::int32_t Code, std::byte* Destination) const;

    const AdcConfig& m_AdcConfig;
    std::size_t m_BytesPerSample = 0;
};
