#pragma once

#include "Config.h"
#include "ISampleQuantizer.h"

#include <cstddef>
#include <cstdint>

class AdcSampleQuantizer final : public ISampleQuantizer
{
  public:
    explicit AdcSampleQuantizer(const AdcConfig& rAdcConfig);
    ~AdcSampleQuantizer() override = default;

    AdcSampleQuantizer(const AdcSampleQuantizer&) = delete;
    AdcSampleQuantizer& operator=(const AdcSampleQuantizer&) = delete;

    void QuantizeSampleAndCopyToMemory(double InputSample, std::byte* pDestination) const override;
    std::size_t GetBytesPerSample() const override;

  private:
    std::uint32_t QuantizeUnipolarCode(double InputSample) const;
    std::int32_t QuantizeBipolarCode(double InputSample) const;
    void WriteUnipolarADCValue(std::uint32_t Code, std::byte* pDestination) const;
    void WriteBipolarADCValue(std::int32_t Code, std::byte* pDestination) const;

    const AdcConfig& m_rAdcConfig;
    std::size_t m_BytesPerSample = 0;
};
