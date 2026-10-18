#pragma once

#include "config/Config.h"
#include "disturbance/IDisturbanceGenerator.h"

#include <cstddef>
#include <cstdint>

class SurgeDisturbanceGenerator final : public IDisturbanceGenerator
{
  public:
    SurgeDisturbanceGenerator(const SurgeEvent& rEvent, std::size_t ChannelCount);
    ~SurgeDisturbanceGenerator() override = default;

    SurgeDisturbanceGenerator(const SurgeDisturbanceGenerator&) = delete;
    SurgeDisturbanceGenerator& operator=(const SurgeDisturbanceGenerator&) = delete;

    void EvaluateDisturbance(std::uint64_t SimulationTimeNs,
                             SampleDisturbance& rOutDisturbance) override;

  private:
    bool IsEventActive(std::uint64_t SimulationTimeNs) const;

    const SurgeEvent& m_rEvent;
    std::size_t m_ChannelCount = 0;
};
