#include <boost/asio.hpp>  
#include <boost/asio/serial_port.hpp>  
#include <iostream>  
#include <string>  
#include <thread>  
#include <chrono>  
#include <stdexcept>  

using namespace boost::asio;  

// Function to send an AT command and read the response  
std::string sendATCommand(serial_port& serial, const std::string& command, int timeout_ms = 1000) {  
    // Write the command to the serial port  
    write(serial, buffer(command + "\r\n"));  
    // Wait for the response  
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));  
    // Read the response  
    std::string response;  
    char c;  
    while (serial.read_some(buffer(&c, 1))) {  
        response += c;  
        if (response.find("OK") != std::string::npos || response.find("ERROR") != std::string::npos) {  
            break;  
        }  
    }  
    return response;  
} 
// Function to handle an incoming call  
void handleIncomingCall(serial_port& serial) {  
    std::cout << "Incoming call detected!" << std::endl;  
    // Answer the call  
    std::cout << "Answering the call..." << std::endl;  
    std::string response = sendATCommand(serial, "ATA");  
    std::cout << "Modem Response: " << response << std::endl;  
    // Play a pre-recorded message (if supported by the modem)  
    std::cout << "Playing pre-recorded message..." << std::endl;  
    response = sendATCommand(serial, "AT+CMEDPLAY=1,\"/home/ronnieji/message.wav\",0");  
    std::cout << "Modem Response: " << response << std::endl;  
    // Wait for the message to finish playing (adjust duration as needed)  
    std::this_thread::sleep_for(std::chrono::seconds(10));  
    // Hang up the call  
    std::cout << "Hanging up the call..." << std::endl;  
    response = sendATCommand(serial, "ATH");  
    std::cout << "Modem Response: " << response << std::endl;  
    std::cout << "Call handled successfully!" << std::endl;  
}  
int main() {  
    try {  
        // Create an io_context for Boost.Asio  
        io_context io;  
        // Open the serial port  
        serial_port serial(io, "/dev/ttyUSB0");  
        // Configure the serial port  
        serial.set_option(serial_port_base::baud_rate(115200));  
        serial.set_option(serial_port_base::character_size(8));  
        serial.set_option(serial_port_base::parity(serial_port_base::parity::none));  
        serial.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::one));  
        serial.set_option(serial_port_base::flow_control(serial_port_base::flow_control::none));  
        // Check modem status  
        std::cout << "Checking modem status..." << std::endl;  
        std::string response = sendATCommand(serial, "AT");  
        if (response.find("OK") == std::string::npos) {  
            throw std::runtime_error("Modem not responding!");  
        }  
        std::cout << "Modem is ready." << std::endl;  
        // Wait for incoming calls  
        std::cout << "Waiting for incoming calls..." << std::endl;  
        while (true) {  
            // Read data from the serial port  
            char c;  
            std::string data;  
            while (serial.read_some(buffer(&c, 1))) {  
                data += c;  
                if (data.find("RING") != std::string::npos) {  
                    handleIncomingCall(serial);  
                    break;  
                }  
            }  

            // Sleep briefly to avoid busy-waiting  
            std::this_thread::sleep_for(std::chrono::milliseconds(100));  
        }  

    } catch (const std::exception& e) {  
        std::cerr << "Error: " << e.what() << std::endl;  
        return 1;  
    }  

    return 0;  
}