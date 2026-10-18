#pragma once

#include "ISampleQuantizer.h"

class Float32SampleQuantizer final : public ISampleQuantizer
{
  public:
    Float32SampleQuantizer() = default;
    ~Float32SampleQuantizer() override = default;

    Float32SampleQuantizer(const Float32SampleQuantizer&) = delete;
    Float32SampleQuantizer& operator=(const Float32SampleQuantizer&) = delete;

    void QuantizeSampleAndCopyToMemory(double InputSample, std::byte* pDestination) const override;
    std::size_t GetBytesPerSample() const override;
};
