#include <iostream>
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/URI.h"


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

class WeatherRequestHandler: public HTTPRequestHandler
{
public:
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) override {
        Poco::URI uri(request.getURI());
        response.setContentType("application/json; charset=utf-8");

        if (uri.getPath() != "/weather") {
            response.setStatus(HTTPResponse::HTTP_NOT_FOUND);
            response.send() << R"({error: not found})";
            return;
        }

        std::string city;
        for (const auto& [key, value] : uri.getQueryParameters()) {
            if (key == "city") city = value;
        }

        if (city.empty()){
            response.setStatus(HTTPResponse::HTTP_BAD_REQUEST);
            response.send() << R"({"error":"city is required"})";
            return;
        }

        std::cout << "Response for city: " << city << std::endl;
        response.setStatus(HTTPResponse::HTTP_OK);
        response.send() << R"({"city":")" << city << R"(","days":[]})";
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
        ServerSocket socket(8080);
        HTTPServer server(new HandlerFactory, socket, new HTTPServerParams);
        server.start();
        std::cout << "Server started on port 8080. Ctrl+C to stop.\n";
        waitForTerminationRequest();
        server.stop();
        return Application::EXIT_OK;
    }
};

POCO_SERVER_MAIN(WeatherServer);