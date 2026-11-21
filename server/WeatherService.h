#pragma once

#include <string>
#include <vector>

#include "WeatherTypes.h"

// Ищет город через геокодинг Open-Meteo. Возвращает false, если город не найден.
bool geocode(const std::string& city, Location& loc);

// Запрашивает прогноз на 5 дней по координатам.
std::vector<DayForecast> getForecast(const Location& loc);
