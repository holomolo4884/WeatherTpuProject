#pragma once

#include <string>

#include <Poco/JSON/Object.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServerResponse.h>

// Отправляет JSON-объект с заданным статусом.
void sendJson(Poco::Net::HTTPServerResponse& response,
              Poco::Net::HTTPResponse::HTTPStatus status,
              const Poco::JSON::Object& obj);

// Отправляет {"error": "<message>"} с заданным статусом.
void sendError(Poco::Net::HTTPServerResponse& response,
               Poco::Net::HTTPResponse::HTTPStatus status,
               const std::string& message);
