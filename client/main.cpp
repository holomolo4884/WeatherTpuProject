// Точка входа клиента: прочитать город, отправить запрос на сервер, вывести результат.

#include <iostream>
#include <string>

#include <Poco/Exception.h>
#include <Poco/JSON/JSONException.h>

#include "InputReader.h"
#include "TablePrinter.h"
#include "WeatherClient.h"

namespace {
const char* const kServerHost = "127.0.0.1";  // IPv4: сервер слушает только IPv4
constexpr unsigned short kServerPort = 8080;
}

int main(int argc, char* argv[]) {
    std::string city = readCity(argc, argv);

    if (city.empty()) {
        std::cerr << "Ошибка: название города не указано" << std::endl;
        return 1;
    }

    try {
        int status = 0;
        std::string body = fetchWeather(kServerHost, kServerPort, buildRequestPath(city), status);
        return printResponse(status, body) ? 0 : 1;
    } catch (const Poco::JSON::JSONException& e) {
        std::cerr << "Некорректный ответ сервера: " << e.displayText() << std::endl;
        return 1;
    } catch (const Poco::Exception& e) {
        std::cerr << "Ошибка соединения с сервером: " << e.displayText() << std::endl;
        return 1;
    }
}
