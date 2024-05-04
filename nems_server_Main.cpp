#include "../lib/nems_server.h"
int main() {
    // Set up the event handler for data received
    setDataReceivedEventHandler([](const char* data) {
        std::cout << "Data Data Data received event handler called with data: " << data << std::endl;
        // Additional processing based on received data
    });
    createServer(8080,"/home/ronnieji/lib/MLCpplib-main/ca_certificates/server.crt","/home/ronnieji/lib/MLCpplib-main/ca_certificates/server.key",5);
    return 0;
}