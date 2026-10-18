#pragma once

#include "config/Config.h"
#include "disturbance/IDisturbanceGenerator.h"

#include <cstddef>
#include <cstdint>

class SagDisturbanceGenerator final : public IDisturbanceGenerator
{
  public:
    SagDisturbanceGenerator(const SagEvent& rEvent, std::size_t ChannelCount);
    ~SagDisturbanceGenerator() override = default;

    SagDisturbanceGenerator(const SagDisturbanceGenerator&) = delete;
    SagDisturbanceGenerator& operator=(const SagDisturbanceGenerator&) = delete;

    void EvaluateDisturbance(std::uint64_t SimulationTimeNs,
                             SampleDisturbance& rOutDisturbance) override;

  private:
    bool IsEventActive(std::uint64_t SimulationTimeNs) const;

    const SagEvent& m_rEvent;
    std::size_t m_ChannelCount = 0;
};
