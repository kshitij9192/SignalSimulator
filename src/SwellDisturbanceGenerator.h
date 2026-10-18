#pragma once

#include "Config.h"
#include "IDisturbanceGenerator.h"

#include <cstddef>
#include <cstdint>

class SwellDisturbanceGenerator final : public IDisturbanceGenerator
{
  public:
    SwellDisturbanceGenerator(const SwellEvent& rEvent, std::size_t ChannelCount);
    ~SwellDisturbanceGenerator() override = default;

    SwellDisturbanceGenerator(const SwellDisturbanceGenerator&) = delete;
    SwellDisturbanceGenerator& operator=(const SwellDisturbanceGenerator&) = delete;

    void EvaluateDisturbance(std::uint64_t SimulationTimeNs,
                             SampleDisturbance& rOutDisturbance) override;

  private:
    bool IsEventActive(std::uint64_t SimulationTimeNs) const;

    const SwellEvent& m_rEvent;
    std::size_t m_ChannelCount = 0;
};
