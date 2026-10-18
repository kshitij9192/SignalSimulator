#pragma once

#include "Config.h"
#include "IDisturbanceGenerator.h"

#include <cstdint>

class FrequencyDisturbanceGenerator final : public IDisturbanceGenerator
{
  public:
    explicit FrequencyDisturbanceGenerator(const FrequencyDistortionEvent& rEvent);
    ~FrequencyDisturbanceGenerator() override = default;

    FrequencyDisturbanceGenerator(const FrequencyDisturbanceGenerator&) = delete;
    FrequencyDisturbanceGenerator& operator=(const FrequencyDisturbanceGenerator&) = delete;

    void EvaluateDisturbance(std::uint64_t SimulationTimeNs,
                             SampleDisturbance& rOutDisturbance) override;

  private:
    bool IsEventActive(std::uint64_t SimulationTimeNs) const;

    const FrequencyDistortionEvent& m_rEvent;
};
