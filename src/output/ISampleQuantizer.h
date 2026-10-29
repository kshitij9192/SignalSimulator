#pragma once

#include <cstddef>

class ISampleQuantizer
{
  public:
    virtual ~ISampleQuantizer() = default;

    virtual void QuantizeSampleAndCopyToMemory(double SampleValue, std::byte* Destination) const = 0;
};
