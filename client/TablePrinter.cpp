#include "TablePrinter.h"

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>

namespace {

// Число символов в UTF-8 строке (а не байтов): продолжающие байты 10xxxxxx не считаются.
std::size_t utf8Length(const std::string& s) {
    std::size_t n = 0;
    for (unsigned char c : s) {
        if ((c & 0xC0) != 0x80) ++n;
    }
    return n;
}

// Дополняет строку пробелами справа до нужной ширины.
std::string padRight(const std::string& s, std::size_t width) {
    std::size_t len = utf8Length(s);
    return len >= width ? s : s + std::string(width - len, ' ');
}

// Дополняет строку пробелами слева (выравнивание по правому краю).
std::string padLeft(const std::string& s, std::size_t width) {
    std::size_t len = utf8Length(s);
    return len >= width ? s : std::string(width - len, ' ') + s;
}

// Число с одним знаком после запятой.
std::string fmt(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << value;
    return out.str();
}

}  // namespace

bool printResponse(int status, const std::string& body) {
    Poco::JSON::Parser parser;
    Poco::Dynamic::Var parsed = parser.parse(body);
    Poco::JSON::Object::Ptr obj = parsed.extract<Poco::JSON::Object::Ptr>();

    if (status != 200) {
        std::string message = obj->optValue<std::string>("error", "неизвестная ошибка");
        std::cerr << "Сервер вернул ошибку (" << status << "): " << message << std::endl;
        return false;
    }

    std::string city = obj->optValue<std::string>("city", "?");
    std::string country = obj->optValue<std::string>("country", "");
    Poco::JSON::Array::Ptr days = obj->getArray("days");

    std::cout << "Прогноз погоды: " << city;
    if (!country.empty()) std::cout << ", " << country;
    std::cout << "\n\n";

    if (!days || days->size() == 0) {
        std::cout << "Нет данных о прогнозе" << std::endl;
        return true;
    }

    const std::size_t wDate = 12, wMin = 10, wMax = 10, wPrecip = 13, wWind = 13;

    std::cout << padRight("Дата", wDate)
              << padLeft("Мин, °C", wMin)
              << padLeft("Макс, °C", wMax)
              << padLeft("Осадки, мм", wPrecip)
              << padLeft("Ветер, км/ч", wWind) << "\n";
    std::cout << std::string(wDate + wMin + wMax + wPrecip + wWind, '-') << "\n";

    for (unsigned int i = 0; i < days->size(); ++i) {
        Poco::JSON::Object::Ptr day = days->getObject(i);
        std::cout << padRight(day->optValue<std::string>("date", "?"), wDate)
                  << padLeft(fmt(day->optValue<double>("tmin", 0.0)), wMin)
                  << padLeft(fmt(day->optValue<double>("tmax", 0.0)), wMax)
                  << padLeft(fmt(day->optValue<double>("precip", 0.0)), wPrecip)
                  << padLeft(fmt(day->optValue<double>("wind", 0.0)), wWind) << "\n";
    }
    std::cout << std::flush;
    return true;
}
