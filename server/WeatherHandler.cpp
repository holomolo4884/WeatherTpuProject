#include "WeatherHandler.h"

#include <chrono>
#include <string>
#include <vector>

#include <Poco/Exception.h>
#include <Poco/URI.h>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/Net/HTTPRequest.h>

#include "JsonResponse.h"
#include "Logger.h"
#include "WeatherService.h"

using namespace Poco::Net;

namespace {

// Достаёт параметр city из строки запроса (пустая строка, если его нет).
std::string extractCity(const Poco::URI& uri) {
    std::string city;
    for (const auto& [key, value] : uri.getQueryParameters()) {
        if (key == "city") city = value;
    }
    return city;
}

// Собирает JSON-ответ по контракту: city, country, latitude, longitude, days[].
Poco::JSON::Object::Ptr buildResult(const Location& loc,
                                    const std::vector<DayForecast>& forecast) {
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

    Poco::JSON::Object::Ptr result = new Poco::JSON::Object;
    result->set("city", loc.name);
    result->set("country", loc.country);
    result->set("latitude", loc.latitude);
    result->set("longitude", loc.longitude);
    result->set("days", days);
    return result;
}

}  // namespace

void WeatherHandler::handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) {
    if (request.getMethod() != HTTPRequest::HTTP_GET) {
        sendError(response, HTTPResponse::HTTP_METHOD_NOT_ALLOWED, "only GET is allowed");
        return;
    }

    Poco::URI uri(request.getURI());
    if (uri.getPath() != "/weather") {
        sendError(response, HTTPResponse::HTTP_NOT_FOUND, "not found");
        return;
    }

    std::string city = extractCity(uri);
    if (city.empty()) {
        sendError(response, HTTPResponse::HTTP_BAD_REQUEST, "city is required");
        return;
    }

    auto start = std::chrono::steady_clock::now();
    auto logDone = [&](const std::string& result) {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now() - start).count();
        logLine("[" + city + "] " + result + " (" + std::to_string(ms) + " мс)");
    };

    try {
        Location loc;
        if (!geocode(city, loc)) {
            logDone("город не найден");
            sendError(response, HTTPResponse::HTTP_NOT_FOUND, "city not found");
            return;
        }

        std::vector<DayForecast> forecast = getForecast(loc);
        sendJson(response, HTTPResponse::HTTP_OK, *buildResult(loc, forecast));
        logDone("OK, дней: " + std::to_string(forecast.size()));
    } catch (const Poco::Exception& e) {
        logDone("ошибка: " + e.displayText());
        sendError(response, HTTPResponse::HTTP_BAD_GATEWAY, "weather service unavailable");
    }
}

HTTPRequestHandler* HandlerFactory::createRequestHandler(const HTTPServerRequest&) {
    return new WeatherHandler;
}
