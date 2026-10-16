#include "ConfigParser.h"
#include "MemoryHandler.h"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

std::string ParseConfigPathFromArguments(int ArgumentCount, char* pArguments[])
{
    std::string ConfigPath;
    for (int ArgumentIndex = 1; ArgumentIndex < ArgumentCount; ++ArgumentIndex)
    {
        const std::string Argument = pArguments[ArgumentIndex];
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
        ConfigPath = pArguments[++ArgumentIndex];
    }

    if (ConfigPath.empty())
    {
        throw std::invalid_argument("usage: signal_simulator --config <path> (or -c <path>)");
    }

    return ConfigPath;
}


int main(int ArgumentCount, char* pArguments[])
{
    if (ArgumentCount == 2 && std::string(pArguments[1]) == "--help")
    {
        std::cout << "Usage: signal_simulator --config <path> (or -c <path>)\n";
        return 0;
    }

    try
    {
        const std::string ConfigPath = ParseConfigPathFromArguments(ArgumentCount, pArguments);
        const SimulatorConfig Config = ConfigParser::ParseConfigFile(ConfigPath);
        MemoryHandler SharedMemory(Config);

        std::cout << "Configuration valid: " << ConfigPath << '\n';
        return 0;
    }
    catch (const std::exception& rError)
    {
        std::cerr << "Configuration error: " << rError.what() << '\n';
        return 1;
    }
}
