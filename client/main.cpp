#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <Poco/URI.h>
#include <Poco/Exception.h>
#include <Poco/StreamCopier.h>
#include <Poco/Timespan.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/JSONException.h>

using namespace Poco::Net;

// Возвращает название города: из аргументов командной строки
// или (если их нет) из ввода пользователя.
std::string readCity(int argc, char* argv[]) {
    std::string city;

    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (i > 1) city += ' ';
            city += argv[i];
        }
    }
    else {
        std::cout << "Введите город: ";
        std::getline(std::cin, city);
    }

    return city;
}

// Собирает путь запроса: /weather?city=<город>.
// Poco::URI сама кодирует пробелы, кириллицу и спецсимволы.
std::string buildRequestPath(const std::string& city) {
    Poco::URI uri;
    uri.setPath("/weather");
    uri.addQueryParameter("city", city);
    return uri.getPathAndQuery();
}

// Отправляет GET-запрос на сервер и возвращает тело ответа.
// В status записывается HTTP-код ответа.
std::string fetchWeather(const std::string& host, unsigned short port, const std::string& path, int& status) {
    HTTPClientSession session(host, port);
    session.setTimeout(Poco::Timespan(30, 0));  // 30 секунд, 0 микросекунд

    HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
    session.sendRequest(request);

    HTTPResponse response;
    std::istream& bodyStream = session.receiveResponse(response);
    status = static_cast<int>(response.getStatus());

    std::string body;
    Poco::StreamCopier::copyToString(bodyStream, body);
    return body;
}

// Число символов в UTF-8 строке (а не байтов): продолжающие байты
// имеют вид 10xxxxxx и не считаются.
size_t utf8Length(const std::string& s) {
    size_t n = 0;
    for (unsigned char c : s) {
        if ((c & 0xC0) != 0x80) ++n;
    }
    return n;
}

// Дополняет строку пробелами справа до нужной ширины.
std::string padRight(const std::string& s, size_t width) {
    size_t len = utf8Length(s);
    return len >= width ? s : s + std::string(width - len, ' ');
}

// Дополняет строку пробелами слева (выравнивание по правому краю).
std::string padLeft(const std::string& s, size_t width) {
    size_t len = utf8Length(s);
    return len >= width ? s : std::string(width - len, ' ') + s;
}

// Число с одним знаком после запятой.
std::string fmt(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << value;
    return out.str();
}

// Разбирает JSON-ответ сервера и печатает результат.
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

    // Ширины колонок
    const size_t wDate = 12, wMin = 10, wMax = 10, wPrecip = 13, wWind = 13;

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

int main(int argc, char* argv[]) {
    std::string city = readCity(argc, argv);

    if (city.empty()) {
        std::cerr << "Ошибка: название города не указано" << std::endl;
        return 1;
    }

    std::string path = buildRequestPath(city);

    try {
        int status = 0;
        std::string body = fetchWeather("127.0.0.1", 8080, path, status);
        return printResponse(status, body) ? 0 : 1;
    } catch (const Poco::JSON::JSONException& e) {
        std::cerr << "Некорректный ответ сервера: " << e.displayText() << std::endl;
        return 1;
    } catch (const Poco::Exception& e) {
        std::cerr << "Ошибка соединения с сервером: " << e.displayText() << std::endl;
        return 1;
    }
}