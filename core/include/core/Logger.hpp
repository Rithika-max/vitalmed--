#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <filesystem>

namespace vetalmed::core {

class Logger {
public:
    enum class Level { DEBUG, INFO, WARN, ERR };
    
    static void init();
    static void log(Level level, const std::string &component, const std::string &message);
    static void debug(const std::string &component, const std::string &message);
    static void info(const std::string &component, const std::string &message);
    static void warn(const std::string &component, const std::string &message);
    static void error(const std::string &component, const std::string &message);

private:
    static std::string levelToString(Level level);
    static std::string getCurrentTimestamp();
};

}
