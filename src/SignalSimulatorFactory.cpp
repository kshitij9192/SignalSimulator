#include "SignalSimulatorFactory.h"

#include "disturbance/FrequencyDisturbanceGenerator.h"
#include "disturbance/SagDisturbanceGenerator.h"
#include "disturbance/SurgeDisturbanceGenerator.h"
#include "disturbance/SwellDisturbanceGenerator.h"
#include "output/AdcSampleQuantizer.h"
#include "output/Float32SampleQuantizer.h"
#include "signal/SineSignalGenerator.h"

#include <stdexcept>
#include <type_traits>
#include <variant>

SignalSimulatorFactory::SignalSimulatorFactory(const SimulatorConfig& Config)
    : m_Config(Config),
      m_OsAbstractions(IOsAbstractions::CreateOsAbstractions()),
      m_MemoryHandler(std::make_unique<MemoryHandler>(m_Config, *m_OsAbstractions)),
      m_SignalGenerator(CreateSignalGenerator(m_Config)),
      m_DisturbanceGenerators(CreateDisturbanceGenerators(m_Config)),
      m_OutputGenerator(std::make_unique<OutputGenerator>(
          m_Config, *m_SignalGenerator, m_DisturbanceGenerators)),
      m_SampleQuantizer(CreateSampleQuantizer(m_Config)),
      m_BlockAssembler(std::make_unique<BlockAssembler>(
          m_Config, *m_OutputGenerator, *m_SampleQuantizer)),
      m_DataProducer(std::make_unique<DataProducer>(
          m_Config, *m_MemoryHandler, *m_OsAbstractions, *m_BlockAssembler))
{
}


DataProducer* SignalSimulatorFactory::GetDataProducer()
{
    return m_DataProducer.get();
}


std::unique_ptr<ISignalGenerator> SignalSimulatorFactory::CreateSignalGenerator(
    const SimulatorConfig& Config)
{
    return std::make_unique<SineSignalGenerator>(Config);
}


std::unique_ptr<IDisturbanceGenerator> SignalSimulatorFactory::CreateDisturbanceGenerator(
    const DisturbanceEvent& Event, std::size_t ChannelCount)
{
    return std::visit(
        [ChannelCount](const auto& ConcreteEvent) -> std::unique_ptr<IDisturbanceGenerator>
        {
            using EventType = std::decay_t<decltype(ConcreteEvent)>;
            if constexpr (std::is_same_v<EventType, SurgeEvent>)
            {
                return std::make_unique<SurgeDisturbanceGenerator>(ConcreteEvent, ChannelCount);
            }
            else if constexpr (std::is_same_v<EventType, SagEvent>)
            {
                return std::make_unique<SagDisturbanceGenerator>(ConcreteEvent, ChannelCount);
            }
            else if constexpr (std::is_same_v<EventType, SwellEvent>)
            {
                return std::make_unique<SwellDisturbanceGenerator>(ConcreteEvent, ChannelCount);
            }
            else
            {
                return std::make_unique<FrequencyDisturbanceGenerator>(ConcreteEvent);
            }
        },
        Event);
}


std::vector<std::unique_ptr<IDisturbanceGenerator>>
SignalSimulatorFactory::CreateDisturbanceGenerators(const SimulatorConfig& Config)
{
    std::vector<std::unique_ptr<IDisturbanceGenerator>> DisturbanceGenerators;
    DisturbanceGenerators.reserve(Config.DisturbanceTimeline.size());
    const std::size_t ChannelCount = Config.Channels.size();

    for (const DisturbanceEvent& Event : Config.DisturbanceTimeline)
    {
        DisturbanceGenerators.push_back(CreateDisturbanceGenerator(Event, ChannelCount));
    }

    return DisturbanceGenerators;
}


std::unique_ptr<ISampleQuantizer> SignalSimulatorFactory::CreateSampleQuantizer(
    const SimulatorConfig& Config)
{
    if (Config.Representation == SampleRepresentation::Float32)
    {
        return std::make_unique<Float32SampleQuantizer>();
    }

    if (!Config.Adc.has_value())
    {
        throw std::invalid_argument("ADC configuration is required for AdcInteger representation");
    }

    return std::make_unique<AdcSampleQuantizer>(*Config.Adc);
}
