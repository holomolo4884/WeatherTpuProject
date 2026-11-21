#pragma once

#include <string>

// Собирает путь запроса /weather?city=<город> (Poco::URI кодирует пробелы и кириллицу).
std::string buildRequestPath(const std::string& city);

// Отправляет GET-запрос на сервер и возвращает тело ответа.
// В status записывается HTTP-код ответа.
std::string fetchWeather(const std::string& host, unsigned short port,
                         const std::string& path, int& status);
