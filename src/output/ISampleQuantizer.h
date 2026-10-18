#pragma once

#include <cstddef>

class ISampleQuantizer
{
  public:
    virtual ~ISampleQuantizer() = default;

    virtual void QuantizeSampleAndCopyToMemory(double SampleValue, std::byte* pDestination) const = 0;
    virtual std::size_t GetBytesPerSample() const = 0;
};
