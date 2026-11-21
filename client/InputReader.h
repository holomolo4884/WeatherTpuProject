#pragma once

#include <string>

// Возвращает название города: из аргументов командной строки
// или (если их нет) из ввода пользователя.
std::string readCity(int argc, char* argv[]);
