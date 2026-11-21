// Точка входа сервера: настройка SSL, запуск HTTPServer на порту 8080.

#include <iostream>
#include <vector>

#include <Poco/SharedPtr.h>
#include <Poco/Net/AcceptCertificateHandler.h>
#include <Poco/Net/Context.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/NetSSL.h>
#include <Poco/Net/SSLManager.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Util/ServerApplication.h>

#include "WeatherHandler.h"

namespace {
constexpr unsigned short kPort = 8080;
}

class WeatherServer : public Poco::Util::ServerApplication {
protected:
    int main(const std::vector<std::string>&) override {
        // SSL нужен для HTTPS-запросов к Open-Meteo.
        // VERIFY_NONE отключает проверку сертификатов (учебный проект).
        Poco::Net::initializeSSL();
        Poco::Net::Context::Ptr context = new Poco::Net::Context(
            Poco::Net::Context::CLIENT_USE, "", Poco::Net::Context::VERIFY_NONE);
        Poco::SharedPtr<Poco::Net::InvalidCertificateHandler> certHandler =
            new Poco::Net::AcceptCertificateHandler(false);
        Poco::Net::SSLManager::instance().initializeClient(nullptr, certHandler, context);

        Poco::Net::ServerSocket socket(kPort);
        Poco::Net::HTTPServer server(new HandlerFactory, socket, new Poco::Net::HTTPServerParams);
        server.start();
        std::cout << "Server started on port " << kPort << ". Ctrl+C to stop." << std::endl;

        waitForTerminationRequest();
        server.stop();

        Poco::Net::uninitializeSSL();
        return Application::EXIT_OK;
    }
};

POCO_SERVER_MAIN(WeatherServer)
