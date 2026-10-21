cmake_minimum_required(VERSION 3.16.0)
project(WeatherTpuProject VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Poco REQUIRED COMPONENTS Foundation Net NetSSL JSON Util)

add_executable(weather_server server/main.cpp)
target_link_libraries(
    weather_server PRIVATE
    Poco::Foundation Poco::Net Poco::NetSSL Poco::JSON Poco::Util
)

add_executable(weather_client client/main.cpp)
target_link_libraries(
    weather_client PRIVATE
    Poco::Foundation Poco::Net Poco::JSON
)