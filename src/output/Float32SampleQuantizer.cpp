#include "output/Float32SampleQuantizer.h"

#include <cstring>

void Float32SampleQuantizer::QuantizeSampleAndCopyToMemory(double InputSample, std::byte* Destination) const
{
    const auto ConvertedSampleValue = static_cast<float>(InputSample);
    std::memcpy(Destination, &ConvertedSampleValue, sizeof(float));
}


std::size_t Float32SampleQuantizer::GetBytesPerSample() const
{
    return sizeof(float);
}
