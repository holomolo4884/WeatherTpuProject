#include "Logger.h"

#include <iostream>
#include <mutex>

void logLine(const std::string& line) {
    static std::mutex m;
    std::lock_guard<std::mutex> lock(m);
    std::cout << line << std::endl;
}
