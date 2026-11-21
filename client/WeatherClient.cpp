#include "WeatherClient.h"

#include <Poco/StreamCopier.h>
#include <Poco/Timespan.h>
#include <Poco/URI.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>

using namespace Poco::Net;

std::string buildRequestPath(const std::string& city) {
    Poco::URI uri;
    uri.setPath("/weather");
    uri.addQueryParameter("city", city);
    return uri.getPathAndQuery();
}

std::string fetchWeather(const std::string& host, unsigned short port,
                         const std::string& path, int& status) {
    HTTPClientSession session(host, port);
    session.setTimeout(Poco::Timespan(30, 0));

    HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
    session.sendRequest(request);

    HTTPResponse response;
    std::istream& bodyStream = session.receiveResponse(response);
    status = static_cast<int>(response.getStatus());

    std::string body;
    Poco::StreamCopier::copyToString(bodyStream, body);
    return body;
}
