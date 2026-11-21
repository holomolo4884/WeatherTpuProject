#pragma once

#include <string>

// Разбирает JSON-ответ сервера и печатает результат: таблицу прогноза
// (status == 200) или сообщение об ошибке в std::cerr.
// Возвращает true, если ответ успешный.
bool printResponse(int status, const std::string& body);
