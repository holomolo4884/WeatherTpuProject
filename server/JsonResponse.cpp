#include "JsonResponse.h"

using namespace Poco::Net;

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
