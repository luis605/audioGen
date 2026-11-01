#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>


enum LogType {
    INFO = 0,
    WARN = 1,
    ERROR = 2,
    FATAL = 3
};

class Logger {
    public:
        void Log(LogType type, const std::string& message) noexcept;
};

#endif //LOGGER_HPP