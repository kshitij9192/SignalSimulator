#pragma once

#include "config/Config.h"
#include "disturbance/IDisturbanceGenerator.h"

#include <cstddef>
#include <cstdint>

class SagDisturbanceGenerator final : public IDisturbanceGenerator
{
  public:
    SagDisturbanceGenerator(const SagEvent& Event, std::size_t ChannelCount);
    ~SagDisturbanceGenerator() override = default;

    SagDisturbanceGenerator(const SagDisturbanceGenerator&) = delete;
    SagDisturbanceGenerator& operator=(const SagDisturbanceGenerator&) = delete;

    void ApplyDisturbance(std::uint64_t SimulationTimeNs,
                          SampleDisturbance& OutDisturbance) override;

  private:
    bool IsEventActive(std::uint64_t SimulationTimeNs) const;

    const SagEvent& m_Event;
    std::size_t m_ChannelCount = 0;
};
