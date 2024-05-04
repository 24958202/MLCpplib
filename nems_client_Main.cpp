#include <iostream>
#include <string>
#include "../lib/nems_client.h"

int main() {
    std::string result;
    result = send_to_server(8080,"127.0.0.1","/home/ronnieji/lib/MLCpplib-main/ca_certificates/ca.crt","Hello world!");
    std::cout << result << '\n';
    return 0;
}
