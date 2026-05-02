#include "core/Logger.hpp"
#include "core/StorageManager.hpp"
#include <fstream>
#include <iostream>
#include <mutex>
#include <iomanip>

namespace fs = std::filesystem;

namespace vetalmed::core {

static std::mutex logMutex;
static std::filesystem::path logFilePath;

void Logger::init() {
    std::lock_guard<std::mutex> lock(logMutex);
    
    logFilePath = StorageManager::logsPath() / "backend.log";
    
    // Create logs directory
    fs::create_directories(StorageManager::logsPath());
    
    // Write initial log
    std::ofstream logFile(logFilePath, std::ios::app);
    if (logFile.is_open()) {
        logFile << "\n=== Backend Started: " << getCurrentTimestamp() << " ===\n";
        logFile.close();
    }
}

std::string Logger::levelToString(Level level) {
    switch (level) {
        case Level::DEBUG: return "DEBUG";
        case Level::INFO: return "INFO";
        case Level::WARN: return "WARN";
        case Level::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void Logger::log(Level level, const std::string &component, const std::string &message) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    try {
        std::string logMessage = "[" + getCurrentTimestamp() + "] [" + levelToString(level) + 
                                "] [" + component + "] " + message;
        
        // Ensure logs directory exists inside configured storage root
        fs::create_directories(StorageManager::logsPath());
        
        // Write to file
        std::ofstream logFile(logFilePath, std::ios::app);
        if (logFile.is_open()) {
            logFile << logMessage << "\n";
            logFile.flush();
            logFile.close();
        }
        
        // Also write to console
        std::cout << logMessage << "\n";
        std::cout.flush();
    } catch (...) {
        // Silently fail if logging fails
        try {
            std::cerr << "Logging error for: " << message << "\n";
        } catch (...) {
            // Give up
        }
    }
}

void Logger::debug(const std::string &component, const std::string &message) {
    log(Level::DEBUG, component, message);
}

void Logger::info(const std::string &component, const std::string &message) {
    log(Level::INFO, component, message);
}

void Logger::warn(const std::string &component, const std::string &message) {
    log(Level::WARN, component, message);
}

void Logger::error(const std::string &component, const std::string &message) {
    log(Level::ERROR, component, message);
}

}
