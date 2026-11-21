#pragma once

#include <string>

// Выполняет HTTPS GET-запрос и возвращает тело ответа.
// При статусе не 200 бросает исключение. При таймауте один раз повторяет запрос.
std::string httpsGet(const std::string& host, const std::string& pathAndQuery);
