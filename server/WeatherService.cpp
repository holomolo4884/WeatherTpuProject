#include "WeatherService.h"

#include <Poco/Exception.h>
#include <Poco/URI.h>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>

#include "HttpsClient.h"

namespace {

// Безопасно достаёт число из массива: если там null или нет элемента, вернёт 0.
double numberAt(const Poco::JSON::Array::Ptr& arr, unsigned int i) {
    if (!arr || i >= arr->size() || arr->isNull(i)) return 0.0;
    return arr->getElement<double>(i);
}

// «Сырой» JSON геокодинга для названия города.
std::string geocodeRaw(const std::string& city) {
    Poco::URI uri;
    uri.setPath("/v1/search");
    uri.addQueryParameter("name", city);
    uri.addQueryParameter("count", "1");
    uri.addQueryParameter("language", "ru");
    return httpsGet("geocoding-api.open-meteo.com", uri.getPathAndQuery());
}

}  // namespace

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

    // В ответе API значения лежат параллельными массивами: элемент i каждого
    // массива относится к одному дню. Собираем их в список структур.
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
