#include "SignalSimulatorFactory.h"
#include "config/ConfigParser.h"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

std::string ParseConfigPathFromArguments(int ArgumentCount, char* Arguments[])
{
    std::string ConfigPath;
    for (int ArgumentIndex = 1; ArgumentIndex < ArgumentCount; ++ArgumentIndex)
    {
        const std::string Argument = Arguments[ArgumentIndex];
        if (Argument != "--config" && Argument != "-c")
        {
            throw std::invalid_argument("unknown argument '" + Argument + "'");
        }
        if (!ConfigPath.empty())
        {
            throw std::invalid_argument("configuration path may only be specified once");
        }
        if (ArgumentIndex + 1 >= ArgumentCount)
        {
            throw std::invalid_argument("missing path after " + Argument);
        }
        ConfigPath = Arguments[++ArgumentIndex];
    }

    if (ConfigPath.empty())
    {
        throw std::invalid_argument("usage: signal_simulator --config <path> (or -c <path>)");
    }

    return ConfigPath;
}


int main(int ArgumentCount, char* Arguments[])
{
    if (ArgumentCount == 2 && std::string(Arguments[1]) == "--help")
    {
        std::cout << "Usage: signal_simulator --config <path> (or -c <path>)\n";
        return 0;
    }

    try
    {
        const std::string ConfigPath = ParseConfigPathFromArguments(ArgumentCount, Arguments);
        const SimulatorConfig Config = ConfigParser::ParseConfigFile(ConfigPath);
        SignalSimulatorFactory SimulatorFactory(Config);
        SimulatorFactory.GetDataProducer()->Run();

        return 0;
    }
    catch (const std::exception& Error)
    {
        std::cerr << "Error: " << Error.what() << '\n';
        return 1;
    }
}
