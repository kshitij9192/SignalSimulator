#pragma once

#include "config/Config.h"
#include "disturbance/IDisturbanceGenerator.h"

#include <cstddef>
#include <cstdint>

class SwellDisturbanceGenerator final : public IDisturbanceGenerator
{
  public:
    SwellDisturbanceGenerator(const SwellEvent& Event, std::size_t ChannelCount);
    ~SwellDisturbanceGenerator() override = default;

    SwellDisturbanceGenerator(const SwellDisturbanceGenerator&) = delete;
    SwellDisturbanceGenerator& operator=(const SwellDisturbanceGenerator&) = delete;

    void ApplyDisturbance(std::uint64_t SimulationTimeNs,
                          SampleDisturbance& OutDisturbance) override;

  private:
    bool IsEventActive(std::uint64_t SimulationTimeNs) const;

    const SwellEvent& m_Event;
    std::size_t m_ChannelCount = 0;
};
