#pragma once

#include <string>

// Найденный город (результат геокодинга).
struct Location {
    std::string name;
    std::string country;
    double latitude = 0.0;
    double longitude = 0.0;
};

// Прогноз на один день.
struct DayForecast {
    std::string date;
    double tmin = 0.0;
    double tmax = 0.0;
    double precip = 0.0;
    double wind = 0.0;
};
