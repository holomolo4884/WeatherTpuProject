#include <iostream>
#include <Poco/Version.h>

int main() {
    std::cout << "POCO version: " << std::hex << POCO_VERSION << std::endl;
}