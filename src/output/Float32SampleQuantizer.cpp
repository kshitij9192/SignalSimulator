#include "output/Float32SampleQuantizer.h"

#include <cstring>

void Float32SampleQuantizer::QuantizeSampleAndCopyToMemory(double InputSample, std::byte* pDestination) const
{
    const float ConvertedSampleValue = static_cast<float>(InputSample);
    std::memcpy(pDestination, &ConvertedSampleValue, sizeof(float));
}


std::size_t Float32SampleQuantizer::GetBytesPerSample() const
{
    return sizeof(float);
}
