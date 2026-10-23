#pragma once

#include "config/Config.h"
#include "disturbance/IDisturbanceGenerator.h"
#include "memory/MemoryHandler.h"
#include "output/BlockAssembler.h"
#include "output/DataProducer.h"
#include "output/ISampleQuantizer.h"
#include "output/OutputGenerator.h"
#include "platform/IOsAbstractions.h"
#include "signal/ISignalGenerator.h"

#include <cstddef>
#include <memory>
#include <vector>

class SignalSimulatorFactory final
{
  public:
    explicit SignalSimulatorFactory(const SimulatorConfig& Config);
    ~SignalSimulatorFactory() = default;

    SignalSimulatorFactory(const SignalSimulatorFactory&) = delete;
    SignalSimulatorFactory& operator=(const SignalSimulatorFactory&) = delete;

    DataProducer* GetDataProducer();

  private:
    static std::unique_ptr<ISignalGenerator> CreateSignalGenerator(
        const SimulatorConfig& Config);
    static std::unique_ptr<IDisturbanceGenerator> CreateDisturbanceGenerator(
        const DisturbanceEvent& Event, std::size_t ChannelCount);
    static std::vector<std::unique_ptr<IDisturbanceGenerator>> CreateDisturbanceGenerators(
        const SimulatorConfig& Config);
    static std::unique_ptr<ISampleQuantizer> CreateSampleQuantizer(
        const SimulatorConfig& Config);

    const SimulatorConfig& m_Config;
    std::unique_ptr<IOsAbstractions> m_OsAbstractions;
    std::unique_ptr<MemoryHandler> m_MemoryHandler;
    std::unique_ptr<ISignalGenerator> m_SignalGenerator;
    std::vector<std::unique_ptr<IDisturbanceGenerator>> m_DisturbanceGenerators;
    std::unique_ptr<OutputGenerator> m_OutputGenerator;
    std::unique_ptr<ISampleQuantizer> m_SampleQuantizer;
    std::unique_ptr<BlockAssembler> m_BlockAssembler;
    std::unique_ptr<DataProducer> m_DataProducer;
};
