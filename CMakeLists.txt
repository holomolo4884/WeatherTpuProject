cmake_minimum_required(VERSION 3.16.0)
project(WeatherTpuProject VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Poco REQUIRED COMPONENTS Foundation Net NetSSL JSON Util)

add_executable(weather_server
    server/main.cpp
    server/WeatherHandler.cpp
    server/WeatherService.cpp
    server/HttpsClient.cpp
    server/JsonResponse.cpp
    server/Logger.cpp)
target_link_libraries(weather_server PRIVATE
    Poco::Foundation Poco::Net Poco::NetSSL Poco::JSON Poco::Util)

add_executable(weather_client
    client/main.cpp
    client/InputReader.cpp
    client/WeatherClient.cpp
    client/TablePrinter.cpp)
target_link_libraries(weather_client PRIVATE
    Poco::Foundation Poco::Net Poco::JSON)

# ---------- Тесты (ctest) ----------
enable_testing()

set(TEST_DIR ${CMAKE_SOURCE_DIR}/tests)
set(BASE "http://127.0.0.1:8080")
set(PIDFILE ${CMAKE_BINARY_DIR}/server.pid)

# Фикстура: сервер запускается до тестов и останавливается после
add_test(NAME server_start
         COMMAND ${TEST_DIR}/server_start.sh $<TARGET_FILE:weather_server> ${PIDFILE})
set_tests_properties(server_start PROPERTIES FIXTURES_SETUP server)

add_test(NAME server_stop
         COMMAND ${TEST_DIR}/server_stop.sh ${PIDFILE})
set_tests_properties(server_stop PROPERTIES FIXTURES_CLEANUP server)

# Вспомогательная функция: HTTP-тест с ожидаемым кодом ответа
function(add_http_test name expected)
    add_test(NAME ${name} COMMAND ${TEST_DIR}/check_http.sh ${expected} ${ARGN})
    set_tests_properties(${name} PROPERTIES FIXTURES_REQUIRED server)
endfunction()

# Тесты сервера (через curl)
add_http_test(http_moscow        200 --get --data-urlencode "city=Moscow" "${BASE}/weather")
add_http_test(http_cyrillic      200 --get --data-urlencode "city=Санкт-Петербург" "${BASE}/weather")
add_http_test(http_space_in_name 200 --get --data-urlencode "city=New York" "${BASE}/weather")
add_http_test(http_unknown_city  404 --get --data-urlencode "city=asdfgh" "${BASE}/weather")
add_http_test(http_empty_city    400 "${BASE}/weather?city=")
add_http_test(http_no_city       400 "${BASE}/weather")
add_http_test(http_bad_path      404 "${BASE}/abc")
add_http_test(http_post_method   405 -X POST "${BASE}/weather?city=Moscow")

# Тесты клиента (запускается сам клиент)
add_test(NAME client_moscow COMMAND weather_client Moscow)
set_tests_properties(client_moscow PROPERTIES
    FIXTURES_REQUIRED server
    PASS_REGULAR_EXPRESSION "Прогноз погоды: Москва")

add_test(NAME client_unknown_city COMMAND weather_client asdfgh)
set_tests_properties(client_unknown_city PROPERTIES
    FIXTURES_REQUIRED server
    PASS_REGULAR_EXPRESSION "city not found")

# Тесты, которым нужен внешний интернет (Open-Meteo)
set_tests_properties(
    http_moscow http_cyrillic http_space_in_name http_unknown_city
    client_moscow client_unknown_city
    PROPERTIES LABELS network)