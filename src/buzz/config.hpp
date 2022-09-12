#pragma once

#include <string>

struct Config {
    std::string windowTitle = "buzz";
    int windowWidth = 1920;
    int windowHeight = 1080;
};

const Config& config();
