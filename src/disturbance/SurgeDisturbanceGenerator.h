#pragma once

#include "config/Config.h"
#include "disturbance/IDisturbanceGenerator.h"

#include <cstddef>
#include <cstdint>

class SurgeDisturbanceGenerator final : public IDisturbanceGenerator
{
  public:
    SurgeDisturbanceGenerator(const SurgeEvent& Event, std::size_t ChannelCount);
    ~SurgeDisturbanceGenerator() override = default;

    SurgeDisturbanceGenerator(const SurgeDisturbanceGenerator&) = delete;
    SurgeDisturbanceGenerator& operator=(const SurgeDisturbanceGenerator&) = delete;

    void ApplyDisturbance(std::uint64_t SimulationTimeNs,
                          SampleDisturbance& OutDisturbance) override;

  private:
    bool IsEventActive(std::uint64_t SimulationTimeNs) const;

    static constexpr double m_Pi = 3.14159265358979323846;

    const SurgeEvent& m_Event;
    std::size_t m_ChannelCount = 0;
};
