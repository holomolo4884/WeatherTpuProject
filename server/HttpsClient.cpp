#include "HttpsClient.h"

#include <Poco/Exception.h>
#include <Poco/StreamCopier.h>
#include <Poco/Timespan.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSClientSession.h>

using namespace Poco::Net;

namespace {

std::string httpsGetOnce(const std::string& host, const std::string& pathAndQuery) {
    HTTPSClientSession session(host, 443);
    session.setTimeout(Poco::Timespan(5, 0));

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

}  // namespace

std::string httpsGet(const std::string& host, const std::string& pathAndQuery) {
    for (int attempt = 1;; ++attempt) {
        try {
            return httpsGetOnce(host, pathAndQuery);
        } catch (const Poco::TimeoutException&) {
            if (attempt >= 2) throw;
        }
    }
}
