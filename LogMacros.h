//
// Created by Bohan Leng on 5/10/2024.
//

#pragma once

#include <iostream>
#include <sstream>
#include <format>
#include <iomanip>
#include <chrono>
#include <ctime>

inline std::string CurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm buf{};
#ifdef _WIN32
    localtime_s(&buf, &in_time_t);
#else
    localtime_r(&in_time_t, &buf);
#endif
    std::stringstream ss;
    ss << std::put_time(&buf, "%Y-%m-%d %X");
    return ss.str();
}

#ifdef _DEBUG
#define DEBUG_MSG(fmt, ...)			std::cout << std::format("[{}] [DEBUG] [{}:{}] ", CurrentTime(), __FUNCTION__, __LINE__) << std::format(fmt, ##__VA_ARGS__) << std::endl
#else
#define DEBUG_MSG(fmt, ...)
#endif

#define INFO_MSG(fmt, ...)			std::cout << std::format("[{}] [INFO] ", CurrentTime()) << std::format(fmt, ##__VA_ARGS__) << std::endl
#define ERROR_MSG(fmt, ...)			std::cerr << std::format("[{}] [ERROR] ", CurrentTime()) << std::format(fmt, ##__VA_ARGS__) << std::endl

