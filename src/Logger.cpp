#include "Logger.hpp"

void Logger::Log(LogType type, const std::string& message) noexcept {
    std::string typeStr;
    switch (type) {
        case INFO:
            typeStr = "INFO: ";
            break;
        case WARN:
            typeStr = "WARN: ";
            break;
        case ERROR:
            typeStr = "ERROR: ";
            break;
        case FATAL:
            typeStr = "FATAL: ";
            break;
        default:
            typeStr = "INFO: ";
    }

    std::cout << typeStr << message << "\n";

    if (type == LogType::FATAL) exit(2);
}