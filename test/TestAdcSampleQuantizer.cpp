#include "config/Config.h"
#include "output/AdcSampleQuantizer.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace
{
template <typename T>
T QuantizeToType(const AdcSampleQuantizer& Quantizer, double InputSample)
{
    std::array<std::byte, sizeof(T)> Buffer{};
    Quantizer.QuantizeSampleAndCopyToMemory(InputSample, Buffer.data());

    T Result{};
    std::memcpy(&Result, Buffer.data(), sizeof(T));
    return Result;
}
} // namespace


TEST_CASE("AdcSampleQuantizer quantizes unipolar samples across all supported bit depths",
          "[AdcSampleQuantizer]")
{
    constexpr double ReferenceAmplitude = 10.0;

    SECTION("8-bit unipolar maps [0, A_ref] to [0, 255] and clamps out-of-range values")
    {
        const AdcConfig Config{ReferenceAmplitude, AdcPolarity::Unipolar, 8};
        const AdcSampleQuantizer Quantizer(Config);

        CHECK(QuantizeToType<std::uint8_t>(Quantizer, -5.0) == 0U);
        CHECK(QuantizeToType<std::uint8_t>(Quantizer, 0.0) == 0U);
        CHECK(QuantizeToType<std::uint8_t>(Quantizer, 5.0) == 128U);
        CHECK(QuantizeToType<std::uint8_t>(Quantizer, 10.0) == 255U);
        CHECK(QuantizeToType<std::uint8_t>(Quantizer, 15.0) == 255U);
    }

    SECTION("12-bit unipolar maps [0, A_ref] to [0, 4095] with upper bits [15:12] zero-extended")
    {
        const AdcConfig Config{ReferenceAmplitude, AdcPolarity::Unipolar, 12};
        const AdcSampleQuantizer Quantizer(Config);

        CHECK(QuantizeToType<std::uint16_t>(Quantizer, -1.0) == 0U);
        CHECK(QuantizeToType<std::uint16_t>(Quantizer, 0.0) == 0U);
        CHECK(QuantizeToType<std::uint16_t>(Quantizer, 5.0) == 2048U);
        CHECK(QuantizeToType<std::uint16_t>(Quantizer, 10.0) == 4095U);

        const std::uint16_t SaturatedCode = QuantizeToType<std::uint16_t>(Quantizer, 20.0);
        CHECK(SaturatedCode == 4095U);
        CHECK((SaturatedCode & 0xF000U) == 0U);
    }

    SECTION("16-bit unipolar maps [0, A_ref] to [0, 65535] and clamps out-of-range values")
    {
        const AdcConfig Config{ReferenceAmplitude, AdcPolarity::Unipolar, 16};
        const AdcSampleQuantizer Quantizer(Config);

        CHECK(QuantizeToType<std::uint16_t>(Quantizer, -1.0) == 0U);
        CHECK(QuantizeToType<std::uint16_t>(Quantizer, 0.0) == 0U);
        CHECK(QuantizeToType<std::uint16_t>(Quantizer, 5.0) == 32768U);
        CHECK(QuantizeToType<std::uint16_t>(Quantizer, 10.0) == 65535U);
        CHECK(QuantizeToType<std::uint16_t>(Quantizer, 12.0) == 65535U);
    }

    SECTION("24-bit unipolar maps [0, A_ref] to [0, 16777215] with upper bits [31:24] zero-extended")
    {
        const AdcConfig Config{ReferenceAmplitude, AdcPolarity::Unipolar, 24};
        const AdcSampleQuantizer Quantizer(Config);

        CHECK(QuantizeToType<std::uint32_t>(Quantizer, -1.0) == 0U);
        CHECK(QuantizeToType<std::uint32_t>(Quantizer, 0.0) == 0U);
        CHECK(QuantizeToType<std::uint32_t>(Quantizer, 5.0) == 8388608U);
        CHECK(QuantizeToType<std::uint32_t>(Quantizer, 10.0) == 16777215U);

        const std::uint32_t SaturatedCode = QuantizeToType<std::uint32_t>(Quantizer, 25.0);
        CHECK(SaturatedCode == 16777215U);
        CHECK((SaturatedCode & 0xFF000000U) == 0U);
    }
}


TEST_CASE("AdcSampleQuantizer quantizes bipolar samples across all supported bit depths",
          "[AdcSampleQuantizer]")
{
    constexpr double ReferenceAmplitude = 10.0;

    SECTION("8-bit bipolar maps [-A_ref, +A_ref] to [-128, 127] without positive-rail wrap-around")
    {
        const AdcConfig Config{ReferenceAmplitude, AdcPolarity::Bipolar, 8};
        const AdcSampleQuantizer Quantizer(Config);

        CHECK(QuantizeToType<std::int8_t>(Quantizer, -15.0) == -128);
        CHECK(QuantizeToType<std::int8_t>(Quantizer, -10.0) == -128);
        CHECK(QuantizeToType<std::int8_t>(Quantizer, 0.0) == 0);
        CHECK(QuantizeToType<std::int8_t>(Quantizer, 10.0) == 127);
        CHECK(QuantizeToType<std::int8_t>(Quantizer, 15.0) == 127);
    }

    SECTION("12-bit bipolar maps [-A_ref, +A_ref] to [-2048, 2047] with sign-extended upper bits")
    {
        const AdcConfig Config{ReferenceAmplitude, AdcPolarity::Bipolar, 12};
        const AdcSampleQuantizer Quantizer(Config);

        CHECK(QuantizeToType<std::int16_t>(Quantizer, -15.0) == -2048);
        CHECK(QuantizeToType<std::int16_t>(Quantizer, -10.0) == -2048);
        CHECK(QuantizeToType<std::int16_t>(Quantizer, 0.0) == 0);
        CHECK(QuantizeToType<std::int16_t>(Quantizer, 10.0) == 2047);
        CHECK(QuantizeToType<std::int16_t>(Quantizer, 15.0) == 2047);
    }

    SECTION("16-bit bipolar maps [-A_ref, +A_ref] to [-32768, 32767] without positive-rail wrap-around")
    {
        const AdcConfig Config{ReferenceAmplitude, AdcPolarity::Bipolar, 16};
        const AdcSampleQuantizer Quantizer(Config);

        CHECK(QuantizeToType<std::int16_t>(Quantizer, -15.0) == -32768);
        CHECK(QuantizeToType<std::int16_t>(Quantizer, -10.0) == -32768);
        CHECK(QuantizeToType<std::int16_t>(Quantizer, 0.0) == 0);
        CHECK(QuantizeToType<std::int16_t>(Quantizer, 10.0) == 32767);
        CHECK(QuantizeToType<std::int16_t>(Quantizer, 15.0) == 32767);
    }

    SECTION("24-bit bipolar maps [-A_ref, +A_ref] to [-8388608, 8388607] with sign-extended upper bits")
    {
        const AdcConfig Config{ReferenceAmplitude, AdcPolarity::Bipolar, 24};
        const AdcSampleQuantizer Quantizer(Config);

        CHECK(QuantizeToType<std::int32_t>(Quantizer, -15.0) == -8388608);
        CHECK(QuantizeToType<std::int32_t>(Quantizer, -10.0) == -8388608);
        CHECK(QuantizeToType<std::int32_t>(Quantizer, 0.0) == 0);
        CHECK(QuantizeToType<std::int32_t>(Quantizer, 10.0) == 8388607);
        CHECK(QuantizeToType<std::int32_t>(Quantizer, 15.0) == 8388607);
    }
}


TEST_CASE("AdcSampleQuantizer writes only the configured container byte width",
          "[AdcSampleQuantizer]")
{
    constexpr std::byte Sentinel{0xAA};

    SECTION("8-bit quantizer writes exactly 1 byte")
    {
        std::array<std::byte, 4> Buffer{Sentinel, Sentinel, Sentinel, Sentinel};
        const AdcConfig Config{5.0, AdcPolarity::Unipolar, 8};
        const AdcSampleQuantizer Quantizer(Config);

        Quantizer.QuantizeSampleAndCopyToMemory(5.0, Buffer.data());

        CHECK(Buffer[0] == std::byte{0xFF});
        CHECK(Buffer[1] == Sentinel);
        CHECK(Buffer[2] == Sentinel);
        CHECK(Buffer[3] == Sentinel);
    }

    SECTION("16-bit quantizer writes exactly 2 bytes")
    {
        std::array<std::byte, 4> Buffer{Sentinel, Sentinel, Sentinel, Sentinel};
        const AdcConfig Config{5.0, AdcPolarity::Unipolar, 16};
        const AdcSampleQuantizer Quantizer(Config);

        Quantizer.QuantizeSampleAndCopyToMemory(5.0, Buffer.data());

        CHECK(Buffer[0] == std::byte{0xFF});
        CHECK(Buffer[1] == std::byte{0xFF});
        CHECK(Buffer[2] == Sentinel);
        CHECK(Buffer[3] == Sentinel);
    }
}
