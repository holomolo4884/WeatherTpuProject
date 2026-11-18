#include <iostream>
#include <string>
#include <Poco/URI.h>
#include <Poco/Exception.h>
#include <Poco/StreamCopier.h>
#include <Poco/Timespan.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>

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
    session.setTimeout(Poco::Timespan(10, 0));  // 10 секунд, 0 микросекунд

    HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
    session.sendRequest(request);

    HTTPResponse response;
    std::istream& bodyStream = session.receiveResponse(response);
    status = static_cast<int>(response.getStatus());

    std::string body;
    Poco::StreamCopier::copyToString(bodyStream, body);
    return body;
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
        std::string body = fetchWeather("localhost", 8080, path, status);

        std::cout << "Статус: " << status << std::endl;
        std::cout << "Ответ: " << body << std::endl;
    }
    catch (const Poco::Exception& e) {
        std::cerr << "Ошибка соединения с сервером: " << e.displayText() << std::endl;
        return 1;
    }

    return 0;
}