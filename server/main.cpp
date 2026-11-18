#include <iostream>
#include <vector>
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/URI.h"
#include <Poco/Exception.h>
#include <Poco/StreamCopier.h>
#include <Poco/Timespan.h>
#include <Poco/SharedPtr.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/SSLManager.h>
#include <Poco/Net/AcceptCertificateHandler.h>
#include <Poco/Net/Context.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Array.h>

// Вводим в текущую область видимости только одно конкретное имя из другого пространства имён
using Poco::Net::HTTPServer;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPRequestHandlerFactory;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::HTTPServerParams;
using Poco::Net::ServerSocket;
using Poco::Util::ServerApplication;

using namespace Poco::Net;


// Выполняет HTTPS GET-запрос и возвращает тело ответа.
// Если статус не 200, бросает исключение.
std::string httpsGet(const std::string& host, const std::string& pathAndQuery) {
    HTTPSClientSession session(host, 443);
    session.setTimeout(Poco::Timespan(10, 0));

    HTTPRequest request(HTTPRequest::HTTP_GET, pathAndQuery, HTTPMessage::HTTP_1_1);
    request.set("User-Agent", "weather-server");
    session.sendRequest(request);

    HTTPResponse response;
    std::istream& bodyStream = session.receiveResponse(response);

    if (response.getStatus() != HTTPResponse::HTTP_OK) {
        throw Poco::RuntimeException("Внешний API вернул статус " +
                                     std::to_string(static_cast<int>(response.getStatus())));
    }

    std::string body;
    Poco::StreamCopier::copyToString(bodyStream, body);
    return body;
}

// Возвращает «сырой» JSON геокодинга для названия города.
std::string geocodeRaw(const std::string& city) {
    Poco::URI uri;
    uri.setPath("/v1/search");
    uri.addQueryParameter("name", city);
    uri.addQueryParameter("count", "1");
    uri.addQueryParameter("language", "ru");
    return httpsGet("geocoding-api.open-meteo.com", uri.getPathAndQuery());
}

struct Location {
    std::string name;
    std::string country;
    double latitude = 0.0;
    double longitude = 0.0;
};

// Ищет город через геокодинг. Возвращает false, если город не найден.
bool geocode(const std::string& city, Location& loc) {
    Poco::JSON::Parser parser;
    Poco::Dynamic::Var parsed = parser.parse(geocodeRaw(city));
    Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();

    // Если город не найден, поля "results" в ответе просто нет.
    Poco::JSON::Array::Ptr results = root->getArray("results");
    if (!results || results->size() == 0) {
        return false;
    }

    Poco::JSON::Object::Ptr first = results->getObject(0);
    loc.name = first->optValue<std::string>("name", city);
    loc.country = first->optValue<std::string>("country", "");
    loc.latitude = first->getValue<double>("latitude");
    loc.longitude = first->getValue<double>("longitude");
    return true;
}

struct DayForecast {
    std::string date;
    double tmin = 0.0;
    double tmax = 0.0;
    double precip = 0.0;
    double wind = 0.0;
};

// Безопасно достаёт число из массива: если там null, вернёт 0.
double numberAt(const Poco::JSON::Array::Ptr& arr, unsigned int i) {
    if (!arr || i >= arr->size() || arr->isNull(i)) return 0.0;
    return arr->getElement<double>(i);
}

// Запрашивает прогноз на 5 дней по координатам.
std::vector<DayForecast> getForecast(const Location& loc) {
    Poco::URI uri;
    uri.setPath("/v1/forecast");
    uri.addQueryParameter("latitude", std::to_string(loc.latitude));
    uri.addQueryParameter("longitude", std::to_string(loc.longitude));
    uri.addQueryParameter("daily",
        "temperature_2m_max,temperature_2m_min,precipitation_sum,wind_speed_10m_max");
    uri.addQueryParameter("forecast_days", "5");
    uri.addQueryParameter("timezone", "auto");

    std::string body = httpsGet("api.open-meteo.com", uri.getPathAndQuery());

    Poco::JSON::Parser parser;
    Poco::Dynamic::Var parsed = parser.parse(body);
    Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
    Poco::JSON::Object::Ptr daily = root->getObject("daily");
    if (!daily) {
        throw Poco::RuntimeException("В ответе прогноза нет поля daily");
    }

    Poco::JSON::Array::Ptr time   = daily->getArray("time");
    Poco::JSON::Array::Ptr tmax   = daily->getArray("temperature_2m_max");
    Poco::JSON::Array::Ptr tmin   = daily->getArray("temperature_2m_min");
    Poco::JSON::Array::Ptr precip = daily->getArray("precipitation_sum");
    Poco::JSON::Array::Ptr wind   = daily->getArray("wind_speed_10m_max");
    if (!time) {
        throw Poco::RuntimeException("В ответе прогноза нет массива time");
    }

    std::vector<DayForecast> days;
    for (unsigned int i = 0; i < time->size(); ++i) {
        DayForecast d;
        d.date   = time->getElement<std::string>(i);
        d.tmax   = numberAt(tmax, i);
        d.tmin   = numberAt(tmin, i);
        d.precip = numberAt(precip, i);
        d.wind   = numberAt(wind, i);
        days.push_back(d);
    }
    return days;
}

void sendJson(HTTPServerResponse& response, HTTPResponse::HTTPStatus status,
              const Poco::JSON::Object& obj) {
    response.setStatusAndReason(status);
    response.setContentType("application/json; charset=utf-8");
    obj.stringify(response.send());
}

void sendError(HTTPServerResponse& response, HTTPResponse::HTTPStatus status,
               const std::string& message) {
    Poco::JSON::Object obj;
    obj.set("error", message);
    sendJson(response, status, obj);
}

class WeatherRequestHandler: public HTTPRequestHandler
{
public:
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) override {
    Poco::URI uri(request.getURI());

    if (uri.getPath() != "/weather") {
        sendError(response, HTTPResponse::HTTP_NOT_FOUND, "not found");
        return;
    }

    std::string city;
    for (const auto& [key, value] : uri.getQueryParameters()) {
        if (key == "city") city = value;
    }

    if (city.empty()) {
        sendError(response, HTTPResponse::HTTP_BAD_REQUEST, "city is required");
        return;
    }

    std::cout << "Request for city: " << city << std::endl;

    try {
        Location loc;
        if (!geocode(city, loc)) {
            std::cout << "  город не найден" << std::endl;
            sendError(response, HTTPResponse::HTTP_NOT_FOUND, "city not found");
            return;
        }

        std::cout << "  найдено: " << loc.name << ", " << loc.country
                  << " (" << loc.latitude << ", " << loc.longitude << ")" << std::endl;

        std::vector<DayForecast> forecast = getForecast(loc);
        std::cout << "  получен прогноз на дней: " << forecast.size() << std::endl;

        Poco::JSON::Array::Ptr days = new Poco::JSON::Array;
        for (const DayForecast& d : forecast) {
            Poco::JSON::Object::Ptr day = new Poco::JSON::Object;
            day->set("date", d.date);
            day->set("tmin", d.tmin);
            day->set("tmax", d.tmax);
            day->set("precip", d.precip);
            day->set("wind", d.wind);
            days->add(day);
        }

        Poco::JSON::Object result;
        result.set("city", loc.name);
        result.set("country", loc.country);
        result.set("latitude", loc.latitude);
        result.set("longitude", loc.longitude);
        result.set("days", days);
        sendJson(response, HTTPResponse::HTTP_OK, result);
    } catch (const Poco::Exception& e) {
        std::cout << "  ошибка: " << e.displayText() << std::endl;
        sendError(response, HTTPResponse::HTTP_BAD_GATEWAY, "weather service unavailable");
    }
}
};

class HandlerFactory : public HTTPRequestHandlerFactory
{
public:
    HTTPRequestHandler* createRequestHandler(const HTTPServerRequest&) override {
        return new WeatherRequestHandler;
    } 
};

class WeatherServer : public Poco::Util::ServerApplication 
{
protected:
    int main (const std::vector<std::string>&) override {
        Poco::Net::initializeSSL();
        Poco::Net::Context::Ptr context = new Poco::Net::Context(
            Poco::Net::Context::CLIENT_USE, "", Poco::Net::Context::VERIFY_NONE);
        Poco::SharedPtr<Poco::Net::InvalidCertificateHandler> certHandler =
            new Poco::Net::AcceptCertificateHandler(false);
        Poco::Net::SSLManager::instance().initializeClient(nullptr, certHandler, context);
        ServerSocket socket(8080);
        HTTPServer server(new HandlerFactory, socket, new HTTPServerParams);
        server.start();
        std::cout << "Server started on port 8080. Ctrl+C to stop.\n";
        waitForTerminationRequest();
        server.stop();
        Poco::Net::uninitializeSSL();
        return Application::EXIT_OK;
    }
};

POCO_SERVER_MAIN(WeatherServer);