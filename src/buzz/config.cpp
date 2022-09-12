#include "config.hpp"

const Config& config()
{
    static Config config;
    return config;
}
