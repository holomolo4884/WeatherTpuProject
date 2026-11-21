#include "InputReader.h"

#include <iostream>

std::string readCity(int argc, char* argv[]) {
    std::string city;

    if (argc > 1) {
        // Склеиваем все аргументы через пробел, чтобы New York работал и без кавычек.
        for (int i = 1; i < argc; ++i) {
            if (i > 1) city += ' ';
            city += argv[i];
        }
    } else {
        std::cout << "Введите город: ";
        std::getline(std::cin, city);
    }

    return city;
}
